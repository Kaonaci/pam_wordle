#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Conversation function
static int test_conv(int num_msg, const struct pam_message **msg,
                    struct pam_response **resp, void *appdata_ptr) {
    int i;
    struct pam_response *response;
    *resp = NULL;
    
    if (num_msg <= 0) return PAM_CONV_ERR;
    
    response = calloc(num_msg, sizeof(struct pam_response));
    if (response == NULL) return PAM_BUF_ERR;
    
    for (i = 0; i < num_msg; i++) {
        switch (msg[i]->msg_style) {
            case PAM_PROMPT_ECHO_OFF:
            case PAM_PROMPT_ECHO_ON:
                printf("%s", msg[i]->msg);
                response[i].resp = malloc(256);
                if (response[i].resp == NULL) {
                    for (int j = 0; j < i; j++) free(response[j].resp);
                    free(response);
                    return PAM_BUF_ERR;
                }
                fgets(response[i].resp, 255, stdin);
                // Убираем символ новой строки
                response[i].resp[strcspn(response[i].resp, "\n")] = 0;
                break;
                
            case PAM_ERROR_MSG:
                fprintf(stderr, "ERROR: %s\n", msg[i]->msg);
                break;
                
            case PAM_TEXT_INFO:
                printf("INFO: %s\n", msg[i]->msg);
                break;
        }
    }
    *resp = response;
    return PAM_SUCCESS;
}

static struct pam_conv conv = {
    &test_conv,
    NULL
};

int main(int argc, char *argv[]) {
    pam_handle_t *pamh = NULL;
    int retval;
    const char *user = "testuser";
    
    if (argc > 1) {
        user = argv[1];
    }
    printf("Testing PAM Wordle module for user: %s\n", user);
    
    retval = pam_start("wordle_test", user, &conv, &pamh);
    if (retval != PAM_SUCCESS) {
        fprintf(stderr, "pam_start failed: %s\n", pam_strerror(pamh, retval));
        return 1;
    }
    retval = pam_authenticate(pamh, 0);
    if (retval != PAM_SUCCESS) {
        fprintf(stderr, "pam_authenticate failed: %s\n", pam_strerror(pamh, retval));
        pam_end(pamh, retval);
        return 1;
    }
    printf("Authentication successful!\n");
    
    retval = pam_end(pamh, retval);
    if (retval != PAM_SUCCESS) {
        fprintf(stderr, "pam_end failed: %s\n", pam_strerror(pamh, retval));
        return 1;
    }
    return 0;
}