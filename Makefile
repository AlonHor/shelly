PROJECT = shelly

CC = gcc

CFLAGS = -Wall -Wextra -Iinclude -g

SRC_DIR = src
OBJ_DIR = build
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET = $(BIN_DIR)/$(PROJECT)

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BIN_DIR):
	mkdir $(BIN_DIR)

.PHONY: all