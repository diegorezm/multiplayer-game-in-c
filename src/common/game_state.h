#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "raylib.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_ALIENS 64
#define MAX_LASERS 128
#define MAX_BLOCKS 256
#define MAX_PLAYERS 2
#define SERVER_PORT 8080
#define SCREEN_WIDTH (750 + 50) // same as client width + offset = 800
#define SCREEN_HEIGHT (700 + 2 * 50)

typedef enum { LASER_NONE = 0, LASER_PLAYER = 1, LASER_ALIEN = 2 } LaserType;

typedef struct {
  Vector2 pos;
  int speed;
  bool active;
  Color color;
  LaserType type;
} Laser;

typedef struct {
  Vector2 pos;
  int type;
  bool alive;
} Alien;

typedef struct {
  Vector2 pos;
  int speed;
  bool alive;
} Spaceship;

typedef struct {
  uint8_t id;
  Vector2 pos;
  int health;
  int score;
  int shipIndex;
  bool connected;
  struct sockaddr_in addr;
} Player;

typedef struct {
  Alien aliens[MAX_ALIENS];
  int alienCount;

  Laser lasers[MAX_LASERS];
  int laserCount;

  Player players[MAX_PLAYERS];
  int playerCount;

  Spaceship ships[MAX_PLAYERS];
  int spaceshipCount;

  int level;
  bool isGameOver;
} GameState;

static inline Laser Laser_Create(Vector2 pos, int speed, Color color,
                                 LaserType type) {
  Laser laser = {pos, speed, true, color, type};
  return laser;
}

void Game_AddLaser(GameState *gs, Vector2 pos, int speed, Color color,
                   LaserType type) {
  if (gs->laserCount >= MAX_LASERS)
    return;

  for (int i = 0; i < MAX_LASERS; i++) {
    if (!gs->lasers[i].active) {
      gs->lasers[i] = Laser_Create(pos, speed, color, type);
      if (i >= gs->laserCount)
        gs->laserCount = i + 1;
      return;
    }
  }
}
#endif
