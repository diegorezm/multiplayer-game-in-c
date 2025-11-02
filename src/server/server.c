#include "game_state.h"
#include "packets.h"
#include "raylib.h"
#include "serialize.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int find_or_add_player(GameState *gs, struct sockaddr_in *addr) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (gs->players[i].connected &&
        gs->players[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
        gs->players[i].addr.sin_port == addr->sin_port)
      return i;
  }

  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (!gs->players[i].connected) {
      Player *p = &gs->players[i];
      p->id = i;
      p->addr = *addr;
      p->score = 0;
      p->health = 3;
      p->connected = true;
      p->shipIndex = i;
      gs->playerCount++;

      Spaceship *s = &gs->ships[i];
      s->alive = true;
      s->speed = 6;
      s->pos = (Vector2){100.0f + i * 300.0f, 600.0f};

      printf("[INFO] Player %d joined (%s:%d)\n", i, inet_ntoa(addr->sin_addr),
             ntohs(addr->sin_port));

      gs->spaceshipCount = gs->playerCount;
      return i;
    }
  }

  printf("[WARN] Server full (max %d players)\n", MAX_PLAYERS);
  return -1;
}

void disconnect_player(GameState *gs, int id) {
  if (id < 0 || id >= MAX_PLAYERS)
    return;

  Player *p = &gs->players[id];
  if (!p->connected)
    return;

  printf("[INFO] Player %d disconnected\n", id);
  p->connected = false;

  Spaceship *s = &gs->ships[id];
  s->alive = false;

  gs->playerCount--;
  gs->spaceshipCount = gs->playerCount;
}

int main() {
  int server_fd;
  struct sockaddr_in server_addr, client_addr;
  server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_fd == -1) {
    perror("Socket creation failed");
    return EXIT_FAILURE;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(SERVER_PORT);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("Bind failed");
    close(server_fd);
    return EXIT_FAILURE;
  }

  printf("UDP server listening on port %d\n", SERVER_PORT);
  socklen_t addr_len = sizeof(client_addr);

  /* GAME */
  GameState game_state = {.level = 1};

  while (1) {
    uint8_t buffer[4096];
    ssize_t bytes_rcvd = recvfrom(server_fd, buffer, sizeof(buffer), 0,
                                  (struct sockaddr *)&client_addr, &addr_len);
    if (bytes_rcvd == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Nothing available right now
      } else {
        perror("recvfrom failed"); // actual network error
      }

    } else if (bytes_rcvd >= (ssize_t)sizeof(PacketHeader)) {
      PacketHeader *hdr = (PacketHeader *)buffer;

      printf("[LOG] From %s:%d | type=%d | size=%d bytes\n",
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
             hdr->type, hdr->size);

      switch (hdr->type) {

      case PKT_JOIN: {
        if (bytes_rcvd < (ssize_t)sizeof(JoinPacket)) {
          fprintf(stderr, "Incomplete join packet (got %zd bytes)\n",
                  bytes_rcvd);
          break;
        }
        JoinPacket *pkt = (JoinPacket *)buffer;
        int id = find_or_add_player(&game_state, &client_addr);

        if (id >= 0) {
          Spaceship *ship = &game_state.ships[id];
          ship->pos = (Vector2){.x = (SCREEN_WIDTH - pkt->spriteWidth) / 2,
                                .y = SCREEN_HEIGHT - pkt->spriteHeight - 100};

          JoinAckPacket ack = JoinAckPacket_New(id);
          sendto(server_fd, &ack, sizeof(ack), 0,
                 (struct sockaddr *)&client_addr, sizeof(client_addr));

          printf("[INFO] Sent JoinAck to %s:%d (id=%d)\n",
                 inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
                 id);
        }
        break;
      }

      case PKT_REQUEST_STATE: {
        if (bytes_rcvd < (ssize_t)sizeof(RequestStatePacket)) {
          fprintf(stderr, "Incomplete request state packet (got %zd bytes)\n",
                  bytes_rcvd);
          break;
        }
        printf("[INFO] Move request state received.\n");
        int id = find_or_add_player(&game_state, &client_addr);
        if (id >= 0) {
          GameStatePacket statePkt = GameStatePacket_New(&game_state);
          sendto(server_fd, &statePkt, sizeof(statePkt), 0,
                 (struct sockaddr *)&client_addr, sizeof(client_addr));
          printf("[INFO] GameState sent to player %d (on request)\n", id);
        }
        break;
      }

      case PKT_MOVE: {
        if (bytes_rcvd < (ssize_t)sizeof(MovePacket)) {
          fprintf(stderr, "Incomplete join packet (got %zd bytes)\n",
                  bytes_rcvd);
          break;
        }
        printf("[INFO] Move packet received.\n");
        MovePacket *pkt = (MovePacket *)buffer;

        int id = find_or_add_player(&game_state, &client_addr);

        if (id >= 0) {
          game_state.ships[id].pos.x = net_to_float(pkt->x_net);
          game_state.ships[id].pos.y = net_to_float(pkt->y_net);
        }
        break;
      }

      case PKT_FIRE: {
        if (bytes_rcvd < (ssize_t)sizeof(FirePacket)) {
          fprintf(stderr, "Incomplete fire packet (got %zd bytes)\n",
                  bytes_rcvd);
          break;
        }
        printf("[INFO] Fire packet received.\n");
        FirePacket *pkt = (FirePacket *)buffer;
        Vector2 start = {net_to_float(pkt->x_net), net_to_float(pkt->y_net)};
        Game_AddLaser(&game_state, start, -8, BLACK, LASER_PLAYER);
        break;
      }

      case PKT_DISCONNECT: {
        if (bytes_rcvd < (ssize_t)sizeof(DisconnectPacket)) {
          fprintf(stderr, "Incomplete join packet (got %zd bytes)\n",
                  bytes_rcvd);
          break;
        }
        printf("[INFO] Disconnect packet received.\n");
        int id = find_or_add_player(&game_state, &client_addr);
        disconnect_player(&game_state, id);
        break;
      }

      default: {
        printf("[WARN] Unknown packet type: %d\n", hdr->type);
        break;
      }
      }
    }

    for (int i = 0; i < game_state.laserCount; i++) {
      Laser *l = &game_state.lasers[i];
      if (!l->active)
        continue;

      l->pos.y += l->speed;

      if (l->pos.y < 0 || l->pos.y > SCREEN_HEIGHT - 100)
        l->active = false;
    }

    GameStatePacket statePkt = GameStatePacket_New(&game_state);
    size_t pktSize = sizeof(statePkt);

    for (size_t i = 0; i < (size_t)game_state.playerCount; i++) {
      Player *p = &game_state.players[i];
      if (p->connected) {
        ssize_t sent = sendto(server_fd, &statePkt, pktSize, 0,
                              (struct sockaddr *)&p->addr, sizeof(p->addr));
        if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
          perror("sendto");
      }
    }

    usleep(16000);
  }

  close(server_fd);
  return 0;
}
