CC := gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -Lexternal/raylib/src
LIBS = -lraylib -lm -ldl -lpthread -lGL -lrt -lX11

SRC_DIR := src
BIN_DIR := bin

SERVER_SRC := $(SRC_DIR)/server.c
CLIENT_SRC := $(SRC_DIR)/client.c

SERVER_BIN := $(BIN_DIR)/server
CLIENT_BIN := $(BIN_DIR)/client

all: $(BIN_DIR) $(SERVER_BIN) $(CLIENT_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) $< -o $@

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -rf $(BIN_DIR)

rebuild: clean all
