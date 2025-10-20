#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[1024] = {0};

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("Socket creation failed");
    return EXIT_FAILURE;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(8080);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("Bind failed");
    return EXIT_FAILURE;
  }

  listen(server_fd, 5);
  printf("Server listening on port 8080\n");

  socklen_t addr_len = sizeof(client_addr);
  client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
  if (client_fd < 0) {
    perror("Accept failed");
    return EXIT_FAILURE;
  }

  while (1) {
    char buffer[1024] = {0};
    if (recv(client_fd, buffer, sizeof(buffer), 0) == -1) {
      perror("Something went wrong.");
    }
    printf("Client: %s\n", buffer);

    char *response = "Message received";

    if (send(client_fd, response, strlen(response), 0) == -1) {
      perror("Something went wrong.");
    }
  }
  return 0;
}
