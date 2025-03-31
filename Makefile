# Variables
NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
SRC_DIR = srcs
INC_DIR = include
OBJ_DIR = obj
TEST_DIR = tests
TEST_CONFIG = test_config.conf

# Find all .cpp files in the srcs directory and subdirectories
SRCS = $(shell find $(SRC_DIR) -type f -name "*.cpp")

# Create a list of corresponding .o files in the obj directory
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Main rule to build the target
all: $(NAME)

# Rule to create the final target
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "\033[0;32mWebServer successfully compiled!\033[0m"

# Rule to compile .cpp files into .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)/$(dir $*)
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

# Rule to clean up generated files
clean:
	rm -rf $(OBJ_DIR)
	@echo "\033[0;33mObject files removed\033[0m"

# Rule to clean up and recompile
fclean: clean
	rm -f $(NAME)
	@echo "\033[0;33mExecutable removed\033[0m"

fc: fclean

# Rule to recompile everything
re: fclean all

# Rule to run configuration tests
test: $(NAME)
	@echo "\033[0;34mRunning configuration tests...\033[0m"
	./$(NAME) --test

# Rule to run comprehensive tests
fulltest: $(NAME)
	@echo "\033[0;34mRunning comprehensive tests...\033[0m"
	./$(NAME) --fulltest

# Rule to run the server with test configuration
runtest: $(NAME)
	@echo "\033[0;34mRunning server with test configuration...\033[0m"
	./$(NAME) $(TEST_CONFIG)

# Rule to run the script-based runtime tests
scripttest: $(NAME)
	@echo "\033[0;34mRunning script-based runtime tests...\033[0m"
	chmod +x ./test_webserver.sh
	./test_webserver.sh

# Show help information
help:
	@echo "\033[0;36mWebServer Makefile commands:\033[0m"
	@echo "  make          - Compile the WebServer"
	@echo "  make clean    - Remove object files"
	@echo "  make fclean   - Remove object files and executable"
	@echo "  make re       - Recompile everything"
	@echo "  make test     - Run configuration tests"
	@echo "  make fulltest - Run comprehensive tests"
	@echo "  make runtest  - Run server with test configuration"
	@echo "  make scripttest - Run script-based runtime tests"

.PHONY: all clean fclean re fc test fulltest runtest scripttest help