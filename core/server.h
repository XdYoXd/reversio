
#ifndef SERVER
#define SERVER

#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <map>
#include <string>

#include "config.h"

struct Connection {
  int fd;
  std::string ip;
};

void forward(int src, int dst, int buffer_size);

class Server {
 private:
  uint16_t port;
  std::map<std::string, std::pair<int, std::time_t>> connections;
  nlohmann::basic_json<> config;

  void Handle(int fd, std::string ip);
  void Forward(int fd, std::string ip);

 public:
  Server(uint16_t port) {
    try {
      this->config = ReadConfig();
    } catch (std::exception& exc) {
      printf("Error: %s\n", exc.what());
      exit(1);
    }

    this->port = port;
  }

  void Run();
};

#endif