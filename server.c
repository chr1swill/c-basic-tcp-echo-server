// #include <sys/types.h>
// #include <signal.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#define PORT "8080"
#define BACKLOG 10
#define MAX_SIZE 1024

void *get_in_addr(struct sockaddr *sa) {
  switch (sa->sa_family) {
    case AF_INET: return &((struct sockaddr_in *)sa)->sin_addr;
    case AF_INET6: return &((struct sockaddr_in6 *)sa)->sin6_addr;;
    default: return NULL;
  }
}

int main(void) {
  int socket_fd = -1, connect_fd = -1;
  struct addrinfo hints = {0}, *result = {0}, *result_p = {0};
  struct sockaddr_storage their_addr = {0};
  socklen_t sin_size = {0};
  int yes = 1;
  int rv = -1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 6 /* TCP in /etc/protocol **/; // no turorial used this but let learn the hard way why they didn't do that 
  hints.ai_flags = AI_PASSIVE; // we set this so it is able to be bind()[ed] since this is a server not a client

  if ((rv = getaddrinfo(NULL, PORT, &hints, &result)) != 0) {
    fprintf(stderr, "Error getting address information: %s\n", gai_strerror(rv));
    return -1;
  }

  for (result_p = result; result_p != NULL; result_p = result_p->ai_next) {
    if ((socket_fd = socket(result_p->ai_family, 
            result_p->ai_socktype, result_p->ai_protocol)) == -1) {
      fprintf(stderr, "Failed to create a socket form address info: %s\n", strerror(errno));
      continue;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      fprintf(stderr, "Failed to set socket: %s\n", strerror(errno));
      close(socket_fd);
      continue;
    }

    if (bind(socket_fd, result_p->ai_addr, result_p->ai_addrlen) == -1) {
      fprintf(stderr, "Failed to bind to socket_fd %d: %s\n", socket_fd, strerror(errno));
      close(socket_fd);
      continue;
    }

    break;
  }
  freeaddrinfo(result);

  if (result_p == NULL) {
    fprintf(stderr, "Error getting valid address info: %s\n", strerror(errno));
    return -1;
  }

  if (listen(socket_fd, BACKLOG) == -1) {
    fprintf(stderr, "Error setting up socket for listening for connections: %s\n", strerror(errno));
    return -1;
  }


  printf("Startng server at 127.0.0.1:8080\n");
  char data_buff[MAX_SIZE];
  while (1) {
    sin_size = sizeof(their_addr);
    if ((connect_fd = accept(socket_fd, 
            (struct sockaddr *)&their_addr, &sin_size)) == -1) {
      fprintf(stderr, "Failed to accept from socket: %s\n", strerror(errno));
      continue;
    }

    const void *in_addr = get_in_addr((struct sockaddr *)&their_addr);
    if (in_addr == NULL) {
      fprintf(stderr, "Their address was invalid\n");
      close(connect_fd);
      break;
    }

    size_t bytes_read;
    const char *exit_command = "exit\r\n";
    for (;;) {
      memset(data_buff, 0, MAX_SIZE);
      if ((bytes_read = read(connect_fd, data_buff, MAX_SIZE)) <= 0) {
        fprintf(stderr, "Failed to read the correct ammount of data from connected fd %d: %s\n", connect_fd, strerror(errno));
        close(connect_fd);
        break;
      }

      if (strncmp(data_buff, exit_command, bytes_read) == 0) {
        printf("Received 'exit command' from client, closing connection\n");
        break;
      };

      if (send(connect_fd, data_buff, MAX_SIZE, 0) <= 0) {
        fprintf(stderr, "Failed to write data to connected fd %d: %s\n", connect_fd, strerror(errno));
        break;
      } 
    }

    close(connect_fd);
  }

  close(socket_fd);
  return 0;
}
