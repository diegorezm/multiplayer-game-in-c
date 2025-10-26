#include "game.h"
#include "raylib.h"
#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MOVE_SPEED 6.0f

int configure_socket(struct sockaddr_in *server_addr) {
  int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (client_fd == -1) {
    perror("Socket creation failed");
    return -1;
  }

  server_addr->sin_family = AF_INET;
  server_addr->sin_port = htons(SERVER_PORT);

  if (inet_pton(AF_INET, "127.0.0.1", &server_addr->sin_addr) <= 0) {
    perror("Invalid server address");
    close(client_fd);
    return -1;
  }

  return client_fd;
}

int send_message(int sock, struct sockaddr_in *addr, const void *data,
                 size_t size) {
  ssize_t sent =
      sendto(sock, data, size, 0, (struct sockaddr *)addr, sizeof(*addr));
  if (sent == -1) {
    perror("sendto failed");
    return -1;
  }
  return 0;
}

size_t receive_message(int sock, struct sockaddr_in *addr, void *buffer,
                       size_t buffer_size) {
  socklen_t addr_len = sizeof(*addr);
  ssize_t bytes_read = recvfrom(sock, buffer, buffer_size, MSG_DONTWAIT,
                                (struct sockaddr *)addr, &addr_len);

  if (bytes_read == -1) {
    perror("recvfrom failed");
    return -1;
  }

  return bytes_read;
}

int main() {
  int client_fd;
  struct sockaddr_in server_addr;

  client_fd = configure_socket(&server_addr);

  struct timeval timeout = {0, 10000}; // 10ms
  setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  if (client_fd == -1)
    return EXIT_FAILURE;

  printf("UDP client ready. Type messages (type 'exit' to quit)\n");

  const int screenWidth = 800;
  const int screenHeight = 450;

  InitWindow(screenWidth, screenHeight, "multiplayer game");

  Player p = {
      .position = {.x = (float)screenWidth / 2, .y = (float)screenHeight / 2}};

  GameState state = {0};

  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    int moved = false;
    MoveCommand cmd;

    if (IsKeyDown(KEY_RIGHT)) {
      p.position.x += MOVE_SPEED;
      moved = true;
    } else if (IsKeyDown(KEY_LEFT)) {
      p.position.x -= MOVE_SPEED;
      moved = true;
    } else if (IsKeyDown(KEY_UP)) {
      p.position.y -= MOVE_SPEED;
      moved = true;
    } else if (IsKeyDown(KEY_DOWN)) {
      p.position.y += MOVE_SPEED;
      moved = true;
    }

    if (moved) {
      cmd = (MoveCommand){.x = p.position.x, .y = p.position.y};
      MoveCommandPacket pkt;
      serialize_move_command(&cmd, &pkt);
      if (send_message(client_fd, &server_addr, &pkt, sizeof(pkt)) != 0) {
        perror("Something went wrong while trying to send the message to the "
               "server.");
      }
    }

    char buffer[sizeof(GameState)];
    ssize_t bytes =
        receive_message(client_fd, &server_addr, buffer, sizeof(buffer));

    if (bytes == sizeof(GameState)) {
      deserialize_game_state(&state, buffer);
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    for (size_t i = 0; i < MAX_PLAYERS; i++) {
      DrawCircleV(state.players[i].position, 15, MAROON);
    }
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
