# commands
CC = gcc
CFLAGS = -Wall -Wextra -g -MMD -MP

# directories
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include

# all sources
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/lexer.c \
       $(SRC_DIR)/parser.c \
       $(SRC_DIR)/utils.c \
       $(SRC_DIR)/testing.c \
#       $(SRC_DIR)/analysis.c

# what files build what and where
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/src/%.o, $(SRCS))
TARGET = $(BUILD_DIR)/tingcc


# build
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)/src
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@


# test
test_lexer:
	@bash testing/lexer/test.sh

test_parser:
	@bash testing/parser/test.sh

# test_analysis:
#	@bash testing/analysis/test.sh


# clean
clean:
	rm -rf $(BUILD_DIR)


.PHONY: all clean test_lexer test_parser #test_analysis
-include $(OBJS:.o=.d)