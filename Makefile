# Variables
MODULE_SRC = pam_wordle.c
MODULE_OBJ = pam_wordle.o
MODULE_SO = pam_wordle.so
TEST_SRC = test_wordle.c
TEST_BIN = test_wordle
WORDS_FILE = words.txt

# System paths (adjust for your distribution)
PAM_DIR = /lib/x86_64-linux-gnu/security
# Choose by your own
CONFIG_DIR = /etc/security/wordle

# Compiler and flags
CC = gcc
CFLAGS = -fPIC -finput-charset=UTF-8 -Wall -Wextra
LDFLAGS = -shared -lpam
TEST_LDFLAGS = -lpam -lpam_misc

# Default target - builds everything
all: module test

# Build PAM module as shared object
module: $(MODULE_SO)

$(MODULE_SO): $(MODULE_OBJ)
	$(CC) $(LDFLAGS) -o $@ $<

$(MODULE_OBJ): $(MODULE_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Build test program
test: $(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CC) -o $@ $< $(TEST_LDFLAGS)

# Install PAM module to system
install-module: $(MODULE_SO)
	@echo "Installing PAM module..."
	sudo mkdir -p $(PAM_DIR)
	sudo cp $(MODULE_SO) $(PAM_DIR)/
	sudo chmod 644 $(PAM_DIR)/$(MODULE_SO)
	@echo "Module installed to $(PAM_DIR)"

# Install words file
install-words: $(WORDS_FILE)
	@echo "Installing words file..."
	sudo mkdir -p $(CONFIG_DIR)
	sudo cp $(WORDS_FILE) $(CONFIG_DIR)/
	sudo chown root:root $(CONFIG_DIR)/$(WORDS_FILE)
	sudo chmod 644 $(CONFIG_DIR)/$(WORDS_FILE)
	@echo "Words file installed to $(CONFIG_DIR)"

# Create example PAM config (optional)
install-config:
	@echo "Creating PAM configuration..."
	@echo "# Wordle PAM configuration" | sudo tee /etc/pam.d/wordle
	@echo "auth required $(MODULE_SO)" | sudo tee -a /etc/pam.d/wordle
	@echo "PAM configuration created at /etc/pam.d/wordle"

# Install everything
install: install-module install-words
	@echo "Installation complete!"
	@echo "Note: You may need to adjust PAM_DIR in Makefile for your distribution"

# Run test program (requires proper PAM setup)
run-test: test
	@echo "Running test program..."
	@echo "Note: Ensure PAM module is installed and configured first"
	./$(TEST_BIN)

# Clean build artifacts
clean:
	rm -f $(MODULE_OBJ) $(MODULE_SO) $(TEST_BIN)
	@echo "Build files cleaned"

# Uninstall from system
uninstall:
	@echo "Uninstalling..."
	sudo rm -f $(PAM_DIR)/$(MODULE_SO)
	sudo rm -rf $(CONFIG_DIR)
	sudo rm -f /etc/pam.d/wordle
	@echo "Uninstallation complete"

# Help target
help:
	@echo "Wordle PAM Module Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all            - Build module and test program (default)"
	@echo "  module         - Build PAM module only"
	@echo "  test           - Build test program only"
	@echo "  install        - Install module and words file to system"
	@echo "  install-module - Install only the PAM module"
	@echo "  install-words  - Install only the words file"
	@echo "  install-config - Create example PAM config"
	@echo "  run-test       - Build and run test program"
	@echo "  clean          - Remove build artifacts"
	@echo "  uninstall      - Remove all installed files"
	@echo "  help           - Show this help message"
	@echo ""
	@echo "Important notes:"
	@echo "  1. Adjust PAM_DIR variable based on your Linux distribution"
	@echo "  2. Test with: ./test_wordle username"
	@echo "  3. Ensure words.txt exists in current directory"

# Phony targets (not actual files)
.PHONY: all module test install install-module install-words install-config \
        run-test clean uninstall help