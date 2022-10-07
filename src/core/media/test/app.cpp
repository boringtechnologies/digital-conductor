#include <arpa/inet.h>
#include <glib-unix.h>
#include <sys/socket.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sync.hpp>
#include <thread>

#define USER_OPTS "1. Add Receiver\n2. Remove Receiver\n3. Stop Sender\n\n"

#define APP_USAGE \
    " <mode> <options> <device>\n\
    mode:\n\
    s - Sender\n\
    r - Receiver\n\n\
    type (optional):\n\
    a - Audio\n\
    v - Video\n\
    (default: v)\n\n\
    device (optional):\n"

GMainLoop *loop;

static gboolean break_handler(gpointer userdata) {
    g_main_loop_quit(loop);
    return TRUE;
}

static int open_socket(const std::string &addr, uint16_t port = 0) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    if (port > 0) {
        struct sockaddr_in server_addr;

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(addr.c_str());

        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
            0) {
            std::cerr << "Couldn't bind to the port\n" << std::endl;
            return -1;
        }
    }

    return sock;
}

static void write_callback(int sock, const std::string &addr,
                           const uint16_t port, const uint8_t *payload,
                           size_t len) {
    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(addr.c_str());

    int ret;
    if ((ret = sendto(sock, payload, len, 0, (struct sockaddr *)&server,
                      sizeof(server))) < 0) {
        std::cout << "Failed to send buf" << std::endl;
    }
    if (len != ret)
        std::cout << strerror(errno) << "(tried, send) " << len << "," << ret
                  << " bytes" << std::endl;
}

void sender_rtcp_recv_thread(media::Sender &sndr, const std::string &id,
                             const std::string &addr, uint16_t port) {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    uint8_t client_msg[2000] = {0};
    struct timeval read_timeout;
    socklen_t client_struct_length = sizeof(client_addr);

    sock = open_socket(addr, port);
    if (sock < 0) {
        std::cerr << "Error creating socket : " << errno << std::endl;
        return;
    }

    while (true) {
        int ret = 0;
        if ((ret = recvfrom(sock, client_msg, sizeof(client_msg), 0,
                            (struct sockaddr *)&client_addr,
                            &client_struct_length)) < 0) {
            std::cerr << "Couldnt receive buffer from the client \n";
            continue;
        }

        sndr.push(id, client_msg, ret);
    }

    close(sock);
}

void sender_cli(media::Sender &sndr) {
    std::map<std::string, std::thread> rtcp_threads;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    while (true) {
        int input;
        std::cout << std::endl << std::endl;
        std::cout << "Please provide input" << std::endl;
        std::cout << USER_OPTS << std::endl;
        std::cin >> input;
        switch (input) {
            case 1: {
                std::string host;
                uint16_t port;
                std::string rcvr_id;
                std::cout << std::endl << std::endl;
                std::cout << "provide host port rcvrid" << std::endl;
                std::cin >> host >> port >> rcvr_id;
                sndr.addReceiver(rcvr_id, [=](const uint8_t *data, size_t len) {
                    write_callback(sock, host, port, data, len);
                });

                // Recieve RTCP packets on all interfaces and on receiver
                // port+1, just incase sender and receiver running on same
                // machine
                std::string sndr_addr = "0.0.0.0";
                rtcp_threads[rcvr_id] =
                    std::thread(sender_rtcp_recv_thread, std::ref(sndr),
                                rcvr_id, sndr_addr, port + 1);
            } break;
            case 2: {
                std::string rcvr_id;
                std::cout << "provide rcvrid" << std::endl;
                std::cin >> rcvr_id;
                sndr.removeReceiver(rcvr_id);

                // TODO: erase causes a SIGABRT, revisit later
                // rtcp_threads.erase(rcvr_id);
            } break;
            case 3:
                sndr.stop();
                close(sock);
                return;
            default:
                std::cout << std::endl << std::endl;
                std::cout << "!!!!!!!!!!!!!!!!!" << std::endl;
                std::cout << "invalid command!" << std::endl;
                std::cout << "!!!!!!!!!!!!!!!!!" << std::endl;
                std::cout << std::endl << std::endl;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(),
                                '\n');
                break;
        }
    }
}

void receiver_thread(media::Receiver &rx, const std::string &addr,
                     uint16_t port) {
    int sock;
    uint8_t client_msg[2000] = {0};
    struct timeval read_timeout;
    struct sockaddr_in client_addr;
    socklen_t client_struct_length = sizeof(client_addr);

    sock = open_socket(addr, port);
    if (sock < 0) {
        return;
    }

    while (true) {
        int ret = 0;
        if ((ret = recvfrom(sock, client_msg, sizeof(client_msg), 0,
                            (struct sockaddr *)&client_addr,
                            &client_struct_length)) < 0) {
            std::cerr << "Couldnt receive buffer from the client \n";
            continue;
        }

        // std::cout << "Received " << ret << " from "
        //           << inet_ntoa(client_addr.sin_addr) << ":"
        //           << ntohs(client_addr.sin_port) << std::endl;
        // std::cout << "-----------------------------------" << std::endl;
        // for (const auto &item : client_msg) {
        //     std::cout << std::hex << int(item);
        // }
        rx.push(client_msg, ret);
        // std::cout << "-----------------------------------" << std::endl;
    }
    close(sock);
}

int main(int argc, char **argv) {
    media::ClockInfo clock = {"66.228.38.186", 10000};
    media::MediaType type = media::MediaType::Video;
    media::CaptureDevice dev;

    if (argc < 2) {
        std::cerr << "Usage:" << std::endl;
        std::cerr << argv[0] << APP_USAGE << std::endl;
        exit(-1);
    }

    if (argc > 2) {
        switch (argv[2][0]) {
            case 'v':
                type = media::MediaType::Video;
                break;

            case 'a':
                type = media::MediaType::Audio;
                break;

            default:
                std::cerr << "Unknown media type " << argv[2] << std::endl;
                std::exit(-1);
        }
    }

    if (argc == 4) {
        dev.name = std::string(argv[3]);
    }

    char mode = argv[1][0];
    if (mode == 's') {
        media::Sender sndr("sender", type, dev, clock);
        sndr.start();

        std::thread cli(sender_cli, std::ref(sndr));
        cli.join();

    } else if (mode == 'r') {
        std::string sender_addr;
        uint16_t port;
        media::PlaybackDevice dev;
        std::chrono::duration<int, std::milli> delay =
            std::chrono::milliseconds(500);
        bool sync = true;
        std::string inp;
        loop = g_main_loop_new(NULL, false);

        std::cout << "Please provide sender address" << std::endl;
        std::cin >> sender_addr;

        std::cout
            << "Using default listen address 0.0.0.0. please provide udp port "
            << std::endl;
        std::cin >> port;

        std::string addr = "0.0.0.0";

        int rtcp_sock = open_socket(addr);
        if (rtcp_sock < 0) {
            return -1;
        }

        media::Receiver recvr(std::to_string(port), type, dev, clock, delay,
                              sync, [=](const uint8_t *data, size_t len) {
                                  write_callback(rtcp_sock, sender_addr,
                                                 port + 1, data, len);
                              });

        recvr.start();
        std::thread th(receiver_thread, std::ref(recvr), addr, port);

        g_unix_signal_add(SIGINT, break_handler, loop);
        std::cout << "Press 'Ctrl-C' to quit" << std::endl;

        // TODO cancel 'th' thread gracefully
        g_main_loop_run(loop);
        close(rtcp_sock);
        recvr.stop();
    } else {
        std::cerr << "Invalid mode!!" << std::endl;
        std::cerr << "Usage:" << std::endl;
        std::cerr << argv[0] << APP_USAGE << std::endl;
    }
    std::cout << "exiting..";
}
