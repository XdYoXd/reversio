#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstdio>
#include <exception>

#include "core/config.h"
#include "core/server.h"

int main() {
  Server server(3333);

  server.Run();
}
