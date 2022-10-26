#include "DiscoveryManager.hpp"

namespace dc
{

#define PORT 12345

void DiscoveryManager::startServer()
{
    int valread;
    struct sockaddr_in address;
    int opt     = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((server_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
};

int DiscoveryManager::connectToServer(std::string hostName)
{

    struct addrinfo hints, *res, *p;
    struct sockaddr_in serv_addr;
    int n;
    int err;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    err               = getaddrinfo(hostName.c_str(), std::to_string(PORT).c_str(), &hints, &res);
    if (err != 0)
    {
        perror("getaddrinfo");
        printf("getaddrinfo %s\n", strerror(errno));
        printf("getaddrinfo : %s \n", gai_strerror(err));
        return 1;
    }

    client_socket = socket(res->ai_family, res->ai_socktype, 0);
    if (client_socket < 0)
    {
        perror("socket");
        return 1;

        const char *ipverstr;
        switch (res->ai_family)
        {
        case AF_INET:
            ipverstr = "IPv4";
            break;
        case AF_INET6:
            ipverstr = "IPv6";
            break;
        default:
            ipverstr = "unknown";
            break;
        }
        printf("ipverstr = %s\n ", ipverstr);
    }

    struct sockaddr_in *addr;

    // loop through all the results and bind to the first we can
    for (p = res; p != NULL; p = p->ai_next)
    {
        addr = (struct sockaddr_in *)p->ai_addr;
        std::cout << "Resolved IPV4 Address of Server - " << inet_ntoa((struct in_addr)addr->sin_addr) << std::endl;
    }

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, inet_ntoa((struct in_addr)addr->sin_addr), &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if ((client_fd = connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    std::string ip_address = inet_ntoa((struct in_addr)addr->sin_addr);

    return 0;
};

std::string DiscoveryManager::receiveGroupList(int socket)
{

    std::cout << "Receiving group list from server - " << std::endl;

    char buffer[1024] = {0};
    read(socket, buffer, 1024);
    std::string message(buffer);

    std::string endLine = "\n";
    std::string comma   = ",";
    std::string equals  = "=";

    size_t el = 0;
    size_t c  = 0;
    size_t eq = 0;

    std::string node;
    std::string section;
    std::string part;

    node_ips.clear();
    node_types.clear();

    while ((el = message.find(endLine)) != std::string::npos)
    {
        node = message.substr(0, el);
        while ((c = node.find(comma)) != std::string::npos)
        {
            section = node.substr(0, c);
            while ((eq = section.find(equals)) != std::string::npos)
            {
                part = section.substr(0, eq);
                section.erase(0, eq + equals.length());
            }
            if (part == "ip-address")
            {
                node_ips.push_back(section);
                std::cout << "    - IP / " << section;
            }
            else if (part == "type")
            {
                node_types.push_back(section);
                std::cout << ", type / " << section << std::endl;
            }
            node.erase(0, c + comma.length());
        }
        message.erase(0, el + endLine.length());
    }

    registered = true;

    return message;
};

void DiscoveryManager::pingServer(std::string group, std::string type, int socket)
{
    std::string message = "{'status':'running','group':'" + group + "','type':'" + type + "'}";
    std::cout << "Ping server with node info - " << message << std::endl;
    send(socket, message.c_str(), strlen(message.c_str()), 0);
};

void DiscoveryManager::unregisterWithServer(std::string group, std::string type, int socket)
{
    std::string message = "{'status':'stopped','group':'" + group + "','type':'" + type + "'}";
    std::cout << "Unregistering node with server - " << message << std::endl;
    send(socket, message.c_str(), strlen(message.c_str()), 0);
};

void DiscoveryManager::closeServerSocket()
{
    // closing the connected socket
    close(server_socket);
    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
}

void DiscoveryManager::closeClientSocket()
{
    // closing the connected socket
    close(client_fd);
}

} // namespace dc
