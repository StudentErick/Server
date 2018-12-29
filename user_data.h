//
// Created by erick on 12/27/18.
//

#ifndef SERVER_USER_DATA_H
#define SERVER_USER_DATA_H

#include <string>
#include <algorithm>
#include <netinet/ip.h>
#include <mutex>
#include <memory>
#include <utility>
#include <set>
#include <string.h>

const unsigned int MAX_PORT = 65535;
const unsigned int MIN_PORT = 1024;
const unsigned int MAX_EVENTS = 1000;
const unsigned int MAX_CHAR_BUFFER = 1000;

struct clientConnection {
    int fd;
    sockaddr_in clientAddr;
    std::shared_ptr<std::string> clientMessage;
    std::shared_ptr<std::mutex> clientMutex;  // 这里仅仅是加锁，不需要条件变量

    clientConnection(int _fd, const sockaddr_in &_addr) {
        fd = _fd;
        clientAddr = _addr;
        clientMessage = std::make_shared<std::string>();
        clientMutex = std::make_shared<std::mutex>();
    }

    bool operator<(const clientConnection &client) const {
        return fd < client.fd;
    }

    bool operator==(const clientConnection &client) const {
        return fd == client.fd;
    }
};

using ConnectionPool = std::set<clientConnection>;

#endif //SERVER_USER_DATA_H
