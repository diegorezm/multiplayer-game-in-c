#ifndef PACKETS_H
#define PACKETS_H
#pragma pack(push, 1)

#include "game_state.h"
#include "serialize.h"
#include <stdint.h>

typedef enum {
  PKT_NONE = 0,
  PKT_JOIN = 1,
  PKT_JOIN_ACK = 2,
  PKT_MOVE = 3,
  PKT_STATE = 4,
  PKT_DISCONNECT = 5,
  PKT_FIRE = 6,
  PKT_REQUEST_STATE = 7
} PacketType;

typedef struct {
  uint8_t type;
  uint8_t version;
  uint16_t size;
} PacketHeader;

static inline PacketHeader PacketHeader_New(PacketType type,
                                            uint16_t payload_size) {
  PacketHeader h;
  h.type = (uint8_t)type;
  h.version = 1;
  h.size = payload_size;
  return h;
}

// ───────────── client → server ─────────────
typedef struct {
  PacketHeader header;
  uint16_t spriteWidth;
  uint16_t spriteHeight;
} JoinPacket;

typedef struct {
  PacketHeader header;
} DisconnectPacket;

typedef struct {
  PacketHeader header;
  uint32_t x_net;
  uint32_t y_net;
} MovePacket;

typedef struct {
  PacketHeader header;
  uint32_t x_net;
  uint32_t y_net;
} FirePacket;

typedef struct {
  PacketHeader header;
} RequestStatePacket;
// ───────────── server → client ─────────────
typedef struct {
  PacketHeader header;
  uint8_t playerId;
} JoinAckPacket;

typedef struct {
  PacketHeader header;
  GameState state;
} GameStatePacket;

// ────────────── Inline constructors

static inline JoinPacket JoinPacket_New(void) {
  JoinPacket p = {0};
  p.header =
      PacketHeader_New(PKT_JOIN, sizeof(JoinPacket) - sizeof(PacketHeader));
  return p;
}

static inline MovePacket MovePacket_New(float x, float y) {
  MovePacket p = {};
  p.header =
      PacketHeader_New(PKT_MOVE, sizeof(MovePacket) - sizeof(PacketHeader));
  p.x_net = float_to_net(x);
  p.y_net = float_to_net(y);
  return p;
}

static inline FirePacket FirePacket_New(float x, float y) {
  FirePacket p = {};
  p.header =
      PacketHeader_New(PKT_FIRE, sizeof(MovePacket) - sizeof(PacketHeader));
  p.x_net = float_to_net(x);
  p.y_net = float_to_net(y);
  return p;
}

static inline GameStatePacket GameStatePacket_New(const GameState *state) {
  GameStatePacket p = {};
  p.header = PacketHeader_New(PKT_STATE,
                              sizeof(GameStatePacket) - sizeof(PacketHeader));
  p.state = *state;
  return p;
}

static inline DisconnectPacket DisconnectPacket_New(void) {
  DisconnectPacket p = {0};
  p.header = PacketHeader_New(PKT_DISCONNECT,
                              sizeof(DisconnectPacket) - sizeof(PacketHeader));
  return p;
}

static inline JoinAckPacket JoinAckPacket_New(uint8_t playerId) {
  JoinAckPacket p = {};
  p.header = PacketHeader_New(PKT_JOIN_ACK,
                              sizeof(JoinAckPacket) - sizeof(PacketHeader));
  p.playerId = playerId;
  return p;
}

static inline RequestStatePacket RequestStatePacket_New() {
  RequestStatePacket p = {};
  p.header = PacketHeader_New(PKT_REQUEST_STATE, sizeof(RequestStatePacket) -
                                                     sizeof(PacketHeader));
  return p;
}

#pragma pack(pop)
#endif // PACKETS_H
