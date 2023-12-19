#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>
#include <thread>

void Server::Run() {
  sockaddr_in sa;
  unsigned long sa_len = sizeof(sa);

  int fd = socket(AF_INET, SOCK_STREAM, 0);

  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = INADDR_ANY;
  sa.sin_port = htons(this->port);

  const int enable = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &enable, sizeof(int)) < 0) {
    perror("error: setsockopt( SO_REUSEADDR | SO_REUSEPORT) failed");
  }

  if (bind(fd, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) == -1) {
    perror("error: bind failed: ");
    return;
  };

  listen(fd, 10);

  printf("Listen %d\n", this->port);
  char ipstr[1000];

  while (1) {
    sockaddr_in ca;
    unsigned int caSize = sizeof(ca);
    char ip[INET_ADDRSTRLEN];

    int c_fd = accept(fd, reinterpret_cast<struct sockaddr *>(&ca), &caSize);
    if (c_fd == -1) {
      perror("Accept failed:");
      continue;
    }

    inet_ntop(AF_INET, &(ca.sin_addr), ip, INET_ADDRSTRLEN);

    std::thread T(&Server::Handle, this, c_fd, ip);
    T.detach();
  }
}

void Server::Handle(int fd, std::string ip) {
  std::time_t currentTime = std::time(nullptr);
  if (connections.find(ip) != connections.end()) {
    if (currentTime - connections[ip].second > static_cast<int>(config["TimePeriod"])) {
      connections[ip] = std::make_pair(1, currentTime);
    } else {
      connections[ip].first++;
    }
  } else {
    connections[ip] = std::make_pair(1, currentTime);
  }

  if (connections[ip].first > static_cast<int>(config["MaxConnections"])) {
    printf("Connection limit reached for %s\n", ip.c_str());
    return;
  }
  printf("New connection from %s\n", ip.c_str());

  this->Forward(fd, ip);
}

void Server::Forward(int fd, std::string ip) {
  /*  Get random server */
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  int numServers = config["Servers"].size();

  int randomIndex = std::rand() % numServers;

  auto randomServer = config["Servers"][randomIndex];

  std::string server_ip = randomServer["Ip"];
  uint16_t server_port = randomServer["Port"];
  int server_timeout = randomServer["Timeout"];

  /* Connect */

  timeval timeout;
  sockaddr_in sa;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("error: socket() failed");
    return;
  }

  timeout.tv_sec = server_timeout;
  timeout.tv_usec = 0;

  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char *>(&timeout), sizeof(timeout)) == -1) {
    perror("error: set sockopt failed");
    return;
  }

  if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char *>(&timeout), sizeof(timeout)) < 0) {
    perror("error: set sockopt failed");
    return;
  }

  sa.sin_family = AF_INET;
  sa.sin_port = htons(server_port);
  inet_pton(AF_INET, server_ip.c_str(), &sa.sin_addr);

  if (connect(sock, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa)) == -1) {
    if (errno == ETIMEDOUT) {
      perror("error: Connection timeout");
    } else {
      perror("error: connection error");
    }
    close(sock);

    return;
  }

  /*  Forward */
  std::thread F1(forward, fd, sock, static_cast<int>(config["BufferSize"]));
  std::thread F2(forward, sock, fd, static_cast<int>(config["BufferSize"]));
  F1.join();
  F2.join();

  close(fd);
  close(sock);
}

void forward(int src, int dst, int buffer_size) {
  char buffer[buffer_size];
  int bytesRead, bytesSent;

  while (1) {
    bytesRead = recv(src, buffer, sizeof(buffer), MSG_NOSIGNAL);
    if (bytesRead <= 0) {
      if (bytesRead == 0) {
        printf("error: connection closed by the other end\n");
      } else {
        printf("error: recv failed: %s\n", strerror(errno));
      }
      return;
    }

    bytesSent = send(dst, buffer, bytesRead, MSG_NOSIGNAL);
    if (bytesSent != bytesRead) {
      printf("error: send failed: %s\n", strerror(errno));
      return;
    }
  }
}
