#pragma once

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>

namespace dc {

class DiscoveryManager {

  public:
    void startServer();
    int connectToServer(std::string host_name);
    std::string receiveGroupList(int socket);
    void pingServer(std::string group, std::string type, int socket);
    void unregisterWithServer(std::string group, std::string type, int socket);
    void closeServerSocket();
    void closeClientSocket();
    int server_socket, server_fd;
    int client_socket, client_fd;
    std::vector<std::string> node_ips;
    std::vector<std::string> node_types;
    bool registered = false;
};

} // namespace dc
