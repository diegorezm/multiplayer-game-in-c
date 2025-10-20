#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  int client_fd;
  struct sockaddr_in server_addr;

  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd == -1) {
    perror("Socket creation failed");
    return EXIT_FAILURE;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);

  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  if (connect(client_fd, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) == -1) {
    perror("Connection failed");
    return EXIT_FAILURE;
  }

  while (1) {
    char message[1024];
    printf("Enter message: ");
    fgets(message, sizeof(message), stdin);

    if (send(client_fd, message, strlen(message), 0) == -1) {
      perror("Something went wrong.");
    }

    char buffer[1024] = {0};
    if (recv(client_fd, buffer, sizeof(buffer), 0) == -1) {
      perror("Something went wrong.");
    }
    printf("Server: %s\n", buffer);
  }
  return 0;
}
