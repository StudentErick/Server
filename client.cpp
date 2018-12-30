#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <strings.h>
#include <arpa/inet.h>
#include <vector>
#include <thread>
#include <atomic>
#include <algorithm>
#include <functional>

const int MAX_CLIENT_NUM = 20;
const int MIN_PORT = 1024;
const int MAX_PORT = 65535;
const int MAX_THREAD_NUMS = 6;

const char *msg[] = {"aaaa", "bbbb", "cccc", "dddd"};

int create_client(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr) < 0) {
        return -1;
    }

    if (connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        return -1;
    }

    return fd;
}

int main(int argc, char *argv[]) {

    const char *ip = "127.0.0.1";
    int port = 8001;
    int num = 4;

    std::atomic_int cnt;
    cnt = 0;

    std::vector<int> clientVec;
    for (int i = 0; i < num; ++i) {
        clientVec.push_back(create_client(ip, port));
    }
    std::vector<std::thread> threads;
    std::cout << "client num = " << num << std::endl;
    for (int i = 0; i < num; ++i) {
        threads.emplace_back([i, &clientVec, &cnt]() {
            auto timeBegin = std::chrono::system_clock::now();
            auto timeNow = timeBegin;
            while (std::chrono::duration_cast<std::chrono::seconds>(timeNow - timeBegin).count() < 5) {
                send(clientVec[i], msg[i], 4, 0);
                std::cout << "send: " << msg[i] << std::endl;
                ++cnt;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                timeNow = std::chrono::system_clock::now();
            }
        });
    }

    std::for_each(threads.begin(), threads.end(),
                  std::mem_fn(&std::thread::join));

    std::cout << "cnt = " << cnt << std::endl;

    return 0;
}