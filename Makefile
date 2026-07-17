CC = gcc
CFLAGS = -Wall -Wextra -g -MMD -MP

BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include

# Only compile these — analysis and codegen are incomplete
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/lexer.c \
	   $(SRC_DIR)/parser.c\
       $(SRC_DIR)/utils.c \
       $(SRC_DIR)/testing.c

OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/src/%.o, $(SRCS))

TARGET = $(BUILD_DIR)/tingcc

# --- Build ---
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)/src
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# --- Test ---
test_lexer:
	@bash testing/lexer/test.sh

test_parser:
	@bash testing/parser/test.sh

# --- Clean ---
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean test_lexer test_parser
-include $(OBJS:.o=.d)