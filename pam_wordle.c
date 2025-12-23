#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/random.h>
#include <syslog.h>

// Tryes limit
#define MAX_ATTEMPTS 7
// Word length
#define LEN 6
// Words file path
#define WORDS_FILE "/etc/security/wordle/words.txt"

// Markers for hints
#define IND_CORRECT   "🟩" // █ ■ ●
#define IND_PRESENT   "🟨" // ░ □ ○
#define IND_ABSENT    "⬜" // - _

// Check that all letters is Latin
int is_only_latin_letters(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!((str[i] >= 'a' && str[i] <= 'z') || 
              (str[i] >= 'A' && str[i] <= 'Z'))) {
            return 0;
        }
    }
    return 1;
}

// Lowercase word
void string_to_lower(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

// Securely get random number within range using kernel CSPRNG
static int get_secure_random(int min, int max) {
    unsigned int random_value;
    ssize_t ret = getrandom(&random_value, sizeof(random_value), GRND_NONBLOCK);
    if (ret != sizeof(random_value)) {
        return -1; // Fallback to predictable value in error case
    }
    return min + (random_value % (max - min + 1));
}

// Load random valid word from file
static int load_random_word(pam_handle_t *pamh, char *buffer, size_t buflen) {
    struct stat st;
    int fd = open(WORDS_FILE, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) {
        pam_error(pamh, "Failed to open words file %s: %s", 
                   WORDS_FILE, strerror(errno));
        return -1;
    }

    // Verify file ownership and permissions
    if (fstat(fd, &st) < 0) {
        close(fd);
        return -1;
    }
    
    // Security check: file must be owned by root and not writable by others
    if (st.st_uid != 0 || (st.st_mode & (S_IWGRP | S_IWOTH))) {
        pam_error(pamh, "Insecure permissions on words file %s", WORDS_FILE);
        close(fd);
        return -1;
    }

    // Convert file descriptor to FILE* stream
    FILE *fp = fdopen(fd, "r");
    if (!fp) {
        pam_error(pamh, "Failed to fdopen words file: %s", strerror(errno));
        close(fd);
        return -1;
    }

    // Count valid words in file
    char line[LEN + 2]; // +2 for newline and null terminator
    int valid_count = 0;
    off_t *positions = NULL;
    
    while (fgets(line, sizeof(line), fp)) {
        // Strip newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        
        // Check if valid word
        if (len == LEN && is_only_latin_letters(line)) {
            // Get current position before reading next line
            off_t pos = ftell(fp) - len - 1;
            
            valid_count++;
            positions = realloc(positions, valid_count * sizeof(off_t));
            if (!positions) {
                pam_error(pamh, "Realloc error while was working with words");
                fclose(fp);
                free(positions);  // Free allocated memory on error
                return -1;
            }
            positions[valid_count - 1] = pos;
        }
    }

    if (valid_count == 0) {
        pam_error(pamh, "No valid words found in %s", WORDS_FILE);
        free(positions);
        fclose(fp);
        return -1;
    }

    // Get random valid word index
    int random_idx = get_secure_random(0, valid_count - 1);
    if (random_idx < 0) {
        pam_syslog(pamh, LOG_WARNING, "Using fallback random selection");
        random_idx = valid_count / 2; // Deterministic fallback
    }

    // Seek to selected word position
    if (fseek(fp, positions[random_idx], SEEK_SET) != 0) {
        pam_error(pamh, "Failed to seek to word position");
        free(positions);
        fclose(fp);
        return -1;
    }
    
    if (fgets(buffer, buflen, fp) == NULL) {
        pam_error(pamh, "Failed to read random word");
        free(positions);
        fclose(fp);
        return -1;
    }
    
    // Clean up
    free(positions);
    fclose(fp);
    
    // Final validation and normalization
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
        len--;
    }
    
    if (len != LEN || !is_only_latin_letters(buffer)) {
        pam_error(pamh, "Selected invalid word from file");
        return -1;
    }
    
    string_to_lower(buffer);
    return 0;
}

int generate_secure_random_word(char *buffer) {
        if (!buffer) {
        errno = EINVAL;
        return -1;
    }
    
    unsigned char random_bytes[LEN];
    size_t bytes_read = 0;
    
    // Безопасное чтение случайных байтов с обработкой EINTR
    while (bytes_read < LEN) {
        ssize_t ret = getrandom(
            random_bytes + bytes_read,
            LEN - bytes_read,
            GRND_NONBLOCK
        );
        
        if (ret < 0) {
            if (errno == EINTR) continue; // Прервано сигналом - повторить
            return -1; // Критическая ошибка
        }
        bytes_read += (size_t)ret;
    }

    // Преобразование в строчные буквы с устранением смещения
    for (int i = 0; i < LEN; i++) {
        // Используем 256 % 26 = 22 "избыточных" значений для устранения смещения
        while (random_bytes[i] >= 234) { // 234 = 256 - (256 % 26)
            if (getrandom(&random_bytes[i], 1, GRND_NONBLOCK) != 1) {
                return -1;
            }
        }
        buffer[i] = 'a' + (random_bytes[i] % 26);
    }
    
    buffer[LEN] = '\0'; // Гарантированное завершение строки
    return 0;
}

int check_word(char* entered_word, const char* hidden_word, char* result) {
    // Check status
    int success = 1;

    // Build result string with emoji indicators
    result[0] = '\0';

    // Frequency array for hidden_word letters (a-z)
    int freq[26] = {0};
    for (int i = 0; i < LEN; i++) {
        char c = hidden_word[i];
        freq[c - 'a']++;
    }

    // Create word hits status
    for (int i = 0; i < LEN; i++) {
        if (freq[entered_word[i] - 'a'] != 0) {
            if (entered_word[i] == hidden_word[i]) {
                // Correct letter and correct position
                strcat(result, IND_CORRECT);
            }
            else {
                // Correct letter and incorrect position
                success = 0;
                strcat(result, IND_PRESENT);
            }
            freq[entered_word[i] - 'a']--;  // Decrement frequency
        }
        else {
            // Incorrect letter
            success = 0;
            strcat(result, IND_ABSENT);
        }
    }

    // Copy to result buffer with proper termination
    result[LEN * 4] = '\0';
    
    return success;
}

static int conversation_func(pam_handle_t *pamh, int msg_style, const char *message, char *output) {
    struct pam_conv *conv;
    struct pam_message msg;
    const struct pam_message *msg_ptr = &msg;
    struct pam_response *resp = NULL;
    int retval;

    msg.msg_style = msg_style;
    msg.msg = message;

    retval = pam_get_item(pamh, PAM_CONV, (const void **)&conv);
    if (retval != PAM_SUCCESS || conv == NULL || conv->conv == NULL) {
        pam_syslog(pamh, LOG_ERR, "Conversation failure");
        return PAM_CONV_ERR;
    }

    retval = conv->conv(1, &msg_ptr, &resp, conv->appdata_ptr);
    if (retval != PAM_SUCCESS) {
        return retval;
    }

    if (resp != NULL) {
        if (output != NULL && resp[0].resp != NULL) {
            // Copy output if it needed
            strncpy(output, resp[0].resp, LEN + 2);
            output[LEN + 1] = '\0';
        }
        // Free memory
        if (resp[0].resp) free(resp[0].resp);
        free(resp);
    }

    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    // Get username
    const char *username;
    int retval = pam_get_user(pamh, &username, NULL);
    if (retval != PAM_SUCCESS) {
        pam_syslog(pamh, LOG_ERR, "Failed to get username");
        return PAM_SYSTEM_ERR;
    }

    // Load random word from file
    char hidden_word[LEN + 2] = {0};;
    if (load_random_word(pamh, hidden_word, sizeof(hidden_word)) != 0) {
        pam_syslog(pamh, LOG_WARNING, "Failed to load random word, using fallback");
        // Stub in case of errors with the file
        generate_secure_random_word(hidden_word);
    }
    
    pam_info(pamh, "Wordle Authentication!");
    pam_info(pamh, "Guess the %d-letter word! You have %d attempts.", 
        LEN, MAX_ATTEMPTS);

    // Initial auth status and buffer for user input
    int success = 0;
    // Buffer for emoji indicators
    char result_buffer[LEN * 4 + 1];

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        // Preparing dialog
        char message1[32];
        snprintf(message1, sizeof(message1), "[ attempt %d/%d ]\nWORD:  ", attempt, MAX_ATTEMPTS);
        // Buffer for entered word
        char entered_word[LEN + 2];

        // Get user input
        int conv_rv = conversation_func(pamh, PAM_PROMPT_ECHO_ON, message1, entered_word);
        if (conv_rv != PAM_SUCCESS) {
            pam_syslog(pamh, LOG_ERR, "Conversation failed: %s", pam_strerror(pamh, conv_rv));
            return conv_rv;
        }

        // Lower string
        string_to_lower(entered_word);
        
        // Check word len and validity
        if (strlen(entered_word) != LEN || !is_only_latin_letters(entered_word)) {
            pam_info(pamh, "Word must be exactly %d Latin letters!", LEN);
            attempt--; // Doesn't count as attempt
            continue;
        }
        
        // Check word hints
        success = check_word(entered_word, hidden_word, result_buffer);

        // Print hint status
        pam_info(pamh, "%s", result_buffer);

        // User guess word
        if (success == 1) {
            pam_info(pamh, "Congratulations! You guessed the word!");
            return PAM_SUCCESS;
        }
    }

    // The attempts are over
    pam_info(pamh, "Game over! The word was: %s", hidden_word);
    return PAM_AUTH_ERR;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}