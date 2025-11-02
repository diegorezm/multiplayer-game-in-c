#include "game_state.h"
#include "packets.h"
#include "raylib.h"
#include <arpa/inet.h>
#include <bits/types/struct_timeval.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MOVE_SPEED 6.0f

static const Color BACKGROUND_COLOR = {240, 237, 229, 255};
static const Color FOREGROUND_COLOR = {49, 47, 44, 255};
static const Color PRIMARY_COLOR = {49, 47, 44, 255};
static const Color RED_COLOR = {234, 39, 39, 255};

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

ssize_t send_message(int sock, const struct sockaddr_in *addr, const void *data,
                     size_t size) {
  if (!data || size == 0)
    return 0;

  ssize_t sent =
      sendto(sock, data, size, 0, (const struct sockaddr *)addr, sizeof(*addr));

  if (sent < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return 0;

    perror("sendto failed");
    return -1;
  }

  if ((size_t)sent != size) {
    fprintf(stderr, "[WARN] Partial packet sent (%zd/%zu bytes)\n", sent, size);
  }

  return sent;
}

size_t receive_message(int sock, struct sockaddr_in *addr, void *buffer,
                       size_t buffer_size) {
  socklen_t addr_len = sizeof(*addr);
  ssize_t bytes_read = recvfrom(sock, buffer, buffer_size, 0,
                                (struct sockaddr *)addr, &addr_len);

  if (bytes_read == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return 0;
    perror("recvfrom failed");
    return (size_t)-1;
  }

  return (size_t)bytes_read;
}

int main() {
  int client_fd;

  struct sockaddr_in server_addr;

  client_fd = configure_socket(&server_addr);

  struct timeval timeout = {0, 01000};
  setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  if (client_fd == -1)
    return EXIT_FAILURE;

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Invaders");
  InitAudioDevice();
  Font font = LoadFontEx("assets/monogram.ttf", 64, 0, 0);
  Texture2D spaceshipImage = LoadTexture("assets/spaceship.png");
  JoinPacket join_pkt = JoinPacket_New();

  join_pkt.spriteHeight = spaceshipImage.height;
  join_pkt.spriteWidth = spaceshipImage.width;

  send_message(client_fd, &server_addr, &join_pkt, sizeof(join_pkt));

  uint8_t buffer[sizeof(JoinAckPacket)];
  size_t bytes =
      receive_message(client_fd, &server_addr, buffer, sizeof(buffer));
  uint8_t myId = 0xFF;

  if (bytes > 0 && bytes >= sizeof(PacketHeader)) {
    PacketHeader *hdr = (PacketHeader *)buffer;
    if (hdr->type == PKT_JOIN_ACK && bytes >= sizeof(JoinAckPacket)) {
      JoinAckPacket *ack = (JoinAckPacket *)buffer;
      myId = ack->playerId;
      printf("Joined server as player %d\n", myId);
    }
  }

  SetTargetFPS(60);

  GameState gs = {0};

  if (myId == 0xFF) {
    fprintf(stderr, "Failed to join server!\n");
    CloseWindow();
    return EXIT_FAILURE;
  }

  double lastPlayerFireTime = 0;
  double lastTimeRequest = 0;

  while (!WindowShouldClose()) {
    double now = GetTime();
    uint8_t buffer[sizeof(GameStatePacket)];
    size_t bytes =
        receive_message(client_fd, &server_addr, buffer, sizeof(buffer));

    if (bytes > 0 && bytes >= sizeof(PacketHeader)) {
      PacketHeader *hdr = (PacketHeader *)buffer;
      if (hdr->type == PKT_STATE) {
        GameStatePacket *pkt = (GameStatePacket *)buffer;
        gs = pkt->state;
        printf("Player count: %d\n", gs.playerCount);
      }
    }

    Spaceship *sp = &gs.ships[myId];
    Vector2 move = sp->pos;
    bool moved = false;

    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
      move.x += sp->speed;
      if (sp->pos.x > GetScreenWidth() - spaceshipImage.width - 20) {
        sp->pos.x = GetScreenWidth() - spaceshipImage.width - 20;
      } else {

        moved = true;
      }
    } else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
      move.x -= sp->speed;

      if (sp->pos.x < 20) {
        sp->pos.x = 20;
      } else {
        moved = true;
      }
    }

    if (IsKeyDown(KEY_SPACE) && now - lastPlayerFireTime > 0.50) {
      FirePacket fire_pkt = FirePacket_New(
          sp->pos.x + spaceshipImage.width / 2 - 2, sp->pos.y - 15);
      send_message(client_fd, &server_addr, &fire_pkt, sizeof(fire_pkt));
    }

    if (moved) {
      sp->pos = move;
      MovePacket move_pkt = MovePacket_New(move.x, move.y);
      send_message(client_fd, &server_addr, &move_pkt, sizeof(move_pkt));
    }

    int hearts = gs.players[myId].health;

    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);
    DrawRectangleRoundedLines((Rectangle){10, 10, 780, 780}, 0.01f, 40,
                              FOREGROUND_COLOR);
    DrawLineEx((Vector2){25, 730}, (Vector2){775, 730}, 2, FOREGROUND_COLOR);

    char levelNumberText[16];
    snprintf(levelNumberText, sizeof(gs.level), "%02d", gs.level);

    char levelText[24];
    sprintf(levelText, "LEVEL %s", levelNumberText);

    DrawTextEx(font, levelText, (Vector2){570, 740}, 34, 2, FOREGROUND_COLOR);

    for (size_t i = 0; i < (size_t)gs.playerCount; i++) {
      Spaceship *ship = &gs.ships[i];
      char playerName[2];
      sprintf(playerName, "%zu", i);

      DrawTextEx(
          font, playerName,
          (Vector2){ship->pos.x + 20, ship->pos.y + spaceshipImage.height}, 18,
          2, FOREGROUND_COLOR);
      DrawTexture(spaceshipImage, ship->pos.x, ship->pos.y, WHITE);
    }

    float x = 50.0;
    for (size_t i = 0; i < (size_t)hearts; i++) {
      DrawTextureV(spaceshipImage, (Vector2){x, 745}, WHITE);
      x += 50;
    }

    for (size_t i = 0; i < (size_t)gs.laserCount; i++) {
      Laser *laser = &gs.lasers[i];
      if (laser->active) {
        DrawRectangleV(laser->pos, (Vector2){4, 15}, laser->color);
      }
    }

    EndDrawing();
  }

  DisconnectPacket disconnect_packet = DisconnectPacket_New();
  send_message(client_fd, &server_addr, &disconnect_packet,
               sizeof(disconnect_packet));

  UnloadTexture(spaceshipImage);
  CloseWindow();
  return 0;
}
