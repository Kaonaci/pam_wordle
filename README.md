# Wordle PAM Module

A security module that replaces traditional password authentication with a Wordle-style word guessing game. Instead of typing a password, you must guess a 6-letter word within 8 attempts using color-coded hints.

## Quick Start

### Build the Module

```bash
make all
```

This creates:
- `pam_wordle.so` - The PAM module
- `test_wordle` - Test program for standalone testing

### Install

```bash
make install
```

This installs:
- Module to `/lib/x86_64-linux-gnu/security/` (adjust path if needed)
- Word list to `/etc/security/wordle/words.txt`

### Uninstall

```bash
make uninstall
```

## How It Works

When authenticating, you're presented with a Wordle game:

```
Wordle Authentication!
Guess the 6-letter word! You have 8 attempts.
[ attempt 1/8 ]
WORD:  travel
⬜🟨🟩⬜🟩🟨
[ attempt 2/8 ]
WORD:  player
🟩🟩🟩🟩🟩🟩
Congratulations! You guessed the word!
```

### Hint System
- 🟩 Green: Correct letter in correct position
- 🟨 Yellow: Correct letter in wrong position  
- ⬜ Gray: Letter not in the word

If you want, you can change the markers in the source code marks.

## Testing

Test the module without system integration:

Make example pam config:
```bash
make install-config
```

Run test:
```bash
make run-test
```

Or manually:
```bash
./test_wordle
```

## Configuration Notes

- Words are randomly selected from `/etc/security/wordle/words.txt`
- 6-letter Latin alphabet words only
- 8 attempts maximum
- Secure random number generation via kernel CSPRNG
- File permission validation (must be owned by root, not writable by others)

For more info:
```bash
make help
```

## Example Session

```
$ ./test_wordle
Testing PAM Wordle module for user: testuser
INFO: Wordle Authentication!
INFO: Guess the 6-letter word! You have 7 attempts.
[ attempt 1/7 ]
WORD:  marker
INFO: ⬜🟨⬜⬜🟨⬜
[ attempt 2/7 ]
WORD:  player
INFO: 🟨🟨🟨⬜🟨⬜
[ attempt 3/7 ]
WORD:  kernel
INFO: ⬜🟨⬜⬜⬜🟩
[ attempt 4/7 ]
WORD:  shadow
INFO: ⬜⬜🟨⬜⬜⬜
[ attempt 5/7 ]
WORD:  poster
INFO: 🟨⬜⬜⬜🟨⬜
[ attempt 6/7 ]
WORD:  module
INFO: ⬜⬜⬜⬜🟨🟨
[ attempt 7/7 ]
WORD:  helper
INFO: ⬜🟨🟨🟨⬜⬜
INFO: Game over! The word was: appeal
pam_authenticate failed: Authentication failure
make: *** [Makefile:71: run-test] Error 1
```

## Customization

Edit `pam_wordle.c` to change:
- `MAX_ATTEMPTS` - Number of allowed guesses
- `LEN` - Word length
- `WORDS_FILE` - Path to word list
- Emoji indicators in the `#define` section

## Security Note

This is a fun educational module. For production systems, use proper authentication methods. The module includes security measures like file permission checks and secure random number generation, but it's not intended as a replacement for strong password policies.
