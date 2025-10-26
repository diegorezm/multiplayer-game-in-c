#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <netinet/in.h>
#include <string.h>

#define SERVER_PORT 8080
#define MAX_PLAYERS 10

typedef struct {
  float x;
  float y;
} MoveCommand;

typedef struct {
  Vector2 position;
  struct sockaddr_in addr;
} Player;

typedef struct {
  uint32_t x_net;
  uint32_t y_net;
} MoveCommandPacket;

typedef struct {
  Player players[MAX_PLAYERS];
} GameState;

size_t serialize_game_state(GameState *state, char *buff) {
  memcpy(buff, state, sizeof(GameState));
  return sizeof(GameState);
}

void deserialize_game_state(GameState *state, const char *buff) {
  memcpy(state, buff, sizeof(GameState));
}

size_t serialize_move_command(const MoveCommand *cmd, MoveCommandPacket *pkt) {
  uint32_t x_raw, y_raw;
  memcpy(&x_raw, &cmd->x, sizeof(float));
  memcpy(&y_raw, &cmd->y, sizeof(float));
  pkt->x_net = htonl(x_raw);
  pkt->y_net = htonl(y_raw);
  return sizeof(MoveCommandPacket);
}

void deserialize_move_command(MoveCommand *cmd, const MoveCommandPacket *pkt) {
  uint32_t x_raw = ntohl(pkt->x_net);
  uint32_t y_raw = ntohl(pkt->y_net);
  memcpy(&cmd->x, &x_raw, sizeof(float));
  memcpy(&cmd->y, &y_raw, sizeof(float));
}

#endif // GAME_H
