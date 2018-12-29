//
// Created by erick on 12/24/18.
//

#include "TcpSocket.h"
#include "user_data.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>
#include <unistd.h>
#include <iostream>

TcpSocket::TcpSocket(int maxWaiter) {
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd < 0) {
        std::cout << "create socket failed\n";
        return;
    }
    m_maxWaiter = maxWaiter;
}

TcpSocket::~TcpSocket() {
    if (m_sockfd >= 0) {
        close(m_sockfd);
    }
}

bool TcpSocket::bindPort(int _port) {
    if (_port < MIN_PORT || _port > MAX_PORT) {
        return false;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(_port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(m_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        return false;
    }
    return true;
}

bool TcpSocket::setServerInfo(const std::string &ip, int _port) {
    auto IP = ip.data();
    bzero(&serv_addr, sizeof(serv_addr));
    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr.s_addr) < 0) {
        return false;
    }
    if (_port < MIN_PORT || _port > MAX_PORT) {
        return false;
    }
    serv_addr.sin_port = htons(_port);
    serv_addr.sin_family = AF_INET;

    return true;
}

bool TcpSocket::connectToHost() {
    if (connect(m_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        return false;
    }
    return true;
}

bool TcpSocket::listenOn() {
    if (listen(m_sockfd, m_maxWaiter) < 0) {
        return false;
    }
    return true;
}

unsigned int TcpSocket::getPort() const {
    auto port = serv_addr.sin_port;
    return static_cast<unsigned int>(port);
}
