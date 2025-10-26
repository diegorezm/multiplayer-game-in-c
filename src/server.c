#include "game.h"
#include <arpa/inet.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

int find_or_add_player(GameState *state, struct sockaddr_in *addr) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    struct sockaddr_in *existing = &state->players[i].addr;

    if (existing->sin_addr.s_addr == addr->sin_addr.s_addr &&
        existing->sin_port == addr->sin_port) {
      return i;
    }
  }

  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (state->players[i].addr.sin_port == 0) {
      state->players[i].addr = *addr;
      state->players[i].position = (Vector2){100.0f + i * 40.0f, 100.0f};
      printf("New player joined: %s:%d (slot %d)\n", inet_ntoa(addr->sin_addr),
             ntohs(addr->sin_port), i);
      return i;
    }
  }

  printf("Server full — cannot add player %s:%d\n", inet_ntoa(addr->sin_addr),
         ntohs(addr->sin_port));
  return -1;
}

int main() {
  int server_fd;
  struct sockaddr_in server_addr, client_addr;
  GameState gs = {0};

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

  while (1) {
    MoveCommandPacket pkt;
    ssize_t bytes_rcvd = recvfrom(server_fd, &pkt, sizeof(pkt), 0,
                                  (struct sockaddr *)&client_addr, &addr_len);

    if (bytes_rcvd > 0 && bytes_rcvd == sizeof(pkt)) {
      MoveCommand cmd;
      deserialize_move_command(&cmd, &pkt);
      int id = find_or_add_player(&gs, &client_addr);
      if (id >= 0) {
        gs.players[id].position.x = cmd.x;
        gs.players[id].position.y = cmd.y;
      }

      printf("Received from %s:%d — moved to (%f, %f)\n",
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
             cmd.x, cmd.y);
    }

    char reply[sizeof(GameState)];
    size_t packet_size = serialize_game_state(&gs, reply);
    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (gs.players[i].addr.sin_port != 0) {
        sendto(server_fd, reply, packet_size, 0,
               (struct sockaddr *)&gs.players[i].addr,
               sizeof(gs.players[i].addr));
      }
    }

    usleep(3300);
  }

  close(server_fd);
  return 0;
}
