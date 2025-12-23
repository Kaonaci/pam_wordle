# Variables
MODULE_FILE = pam_wordle
TEST_FILE = test_wordle
WORDS_FILE = words.txt
PAM_DIR = /lib/x86_64-linux-gnu/security
CONFIG_DIR = /etc/security/wordle

# Default target
all: module install test words

# Build PAM module
module: $(MODULE_FILE).so

$(MODULE_FILE).so: $(MODULE_FILE).o
	gcc -shared -o $(MODULE_FILE).so $(MODULE_FILE).o -lpam

$(MODULE_FILE).o: $(MODULE_FILE).c
	gcc -fPIC -finput-charset=UTF-8 -c $(MODULE_FILE).c -o $(MODULE_FILE).o

# Install module
install-module: $(MODULE_FILE).so
	sudo cp $(MODULE_FILE).so $(PAM_DIR)/
	sudo chmod 644 $(PAM_DIR)/$(MODULE_FILE).so

# Build test program
test: $(TEST_FILE)

$(TEST_FILE): $(TEST_FILE).c
	gcc -o $(TEST_FILE) $(TEST_FILE).c -lpam

# Install words file
words:
	sudo mkdir -p $(CONFIG_DIR)
	sudo cp $(WORDS_FILE) $(CONFIG_DIR)/$(WORDS_FILE)
	sudo chown root:root $(CONFIG_DIR)/$(WORDS_FILE)
	sudo chmod 644 $(CONFIG_DIR)/$(WORDS_FILE)

# Install everything
install: install-module words

# Run test
run: test
	./$(TEST_FILE)

# Clean build files
clean:
	rm -f $(MODULE_FILE).o $(MODULE_FILE).so $(TEST_FILE)

# Uninstall from system
uninstall:
	sudo rm -f $(PAM_DIR)/$(MODULE_FILE).so
	sudo rm -rf $(CONFIG_DIR)

# Help
help:
	@echo "Available targets:"
	@echo "  all          - Build everything (module, test)"
	@echo "  module       - Build PAM module (.so)"
	@echo "  install-module - Install module to system"
	@echo "  test         - Build test program"
	@echo "  words        - Install words file to system"
	@echo "  install      - Install everything (module + words)"
	@echo "  run-test     - Build and run test"
	@echo "  clean        - Remove build files"
	@echo "  uninstall    - Remove from system"
	@echo "  help         - Show this help"

# Phony targets (not real files)
.PHONY: all module install-module test words install run-test clean uninstall help