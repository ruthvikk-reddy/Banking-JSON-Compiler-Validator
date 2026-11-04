CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -Iinclude -Isrc
BISON = bison
FLEX = flex

TARGET = banking_validator

# Complete 8-Stage Architecture Sources
SRC = src/json_value.c \
      src/pattern_matcher.c \
      src/domain_rules.c \
      src/semantic_analyzer.c \
      src/cross_field_validator.c \
      src/error_reporter.c \
      src/validation_report.c \
      src/main.c

PARSER_SRC = src/json_parser.tab.c
PARSER_HDR = src/json_parser.tab.h
LEXER_SRC  = src/lex.yy.c

OBJS = $(SRC:.c=.o) src/json_parser.tab.o src/lex.yy.o

.PHONY: all clean test help

all: $(TARGET)

$(TARGET): $(PARSER_HDR) $(PARSER_SRC) $(LEXER_SRC) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(PARSER_SRC) $(PARSER_HDR): src/json_parser.y
	$(BISON) -d -o src/json_parser.tab.c src/json_parser.y

$(LEXER_SRC): src/json_lexer.l $(PARSER_HDR)
	$(FLEX) -o src/lex.yy.c src/json_lexer.l

src/json_parser.tab.o: src/json_parser.tab.c $(PARSER_HDR)
	$(CC) $(CFLAGS) -c $< -o $@

src/lex.yy.o: src/lex.yy.c $(PARSER_HDR)
	$(CC) $(CFLAGS) -c $< -o $@

src/%.o: src/%.c $(PARSER_HDR)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	@chmod +x tests/test_runner.sh
	@./tests/test_runner.sh

clean:
	rm -f $(TARGET) $(TARGET).exe src/*.o $(PARSER_SRC) $(PARSER_HDR) $(LEXER_SRC)

help:
	@echo "Available make targets:"
	@echo "  make         - Compile the banking validator application (Stages 1–8 complete)"
	@echo "  make test    - Run the automated test suite"
	@echo "  make clean   - Remove generated artifacts and binary"
	@echo "  make help    - Show this help message"
