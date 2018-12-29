//
// Created by erick on 12/27/18.
//

#include <cstring>
#include <ios>
#include <utility>
#include <fcntl.h>
#include "MyServer.h"

MyServer::MyServer(int _proNums, int _conNums, int _maxWaiters) :
        TcpServer(_maxWaiters) {

    m_bWriteBlock = false;
    m_iOpeNum = 0;
    m_bPoolBlock = false;
    m_bStop = false;

    m_producerThreads = std::make_unique<ThreadPool>(_proNums);
    m_consumerThreads = std::make_unique<ThreadPool>(_conNums);

    m_fileStream = std::make_unique<std::fstream>("./log.txt", std::ios::app);

    //m_fileStream->open("./log.txt", std::ios::app | std::ios::in);
    if (m_fileStream->is_open()) {
        *(m_fileStream) << "logger test\n";
    } else {
        std::cout << "open file failed\n";
        return;
    }


    // 只要服务器开始运行，写文件的线程就处于就绪等待的状态
    m_consumerThreads->enqueue([this]() {
        while(!m_bStop) {  // 一定要记住这里的死循环，一次自动机模型。。。WTF,,MMP,,坑死劳资了。。。。
            std::unique_lock<std::mutex> lock(m_mtxWrite);
            m_condWrite.wait(lock, [this]() -> bool {  // 消息队列不空，整个连接池没有被锁住
                return !m_qMsgQue.empty() && !m_bPoolBlock && !m_bWriteBlock;
            });
            ++m_iOpeNum;  // 操作的个数+1
            m_bWriteBlock = true;
            while (!m_qMsgQue.empty()) {
                int fd = m_qMsgQue.pop();
                sockaddr_in addr;
                bzero(&addr, sizeof(addr));
                auto it = m_connectionPool.find(clientConnection(fd, addr));
                if (it == m_connectionPool.end()) {
                    continue;
                }

                it->clientMutex->lock();  // 读取数据要加锁
                /*
                 * 在这里需要添加/n的判别，但是先进行测试，不管那么多
                 */
                std::string str = std::move(*it->clientMessage);
                *(m_fileStream) << str;  // 写入文件
                std::cout << "-------------file:" << str << std::endl;///////////////////////////测试
                it->clientMutex->unlock();
            }
            --m_iOpeNum;  // 操作的个数-1
            m_bWriteBlock = false;
            m_condPool.notify_all();  // 唤醒写读写整个连接池的操作
        }
    });
}

MyServer::~MyServer() {
    m_fileStream->close();
}

void MyServer::newConnection() {
    sockaddr_in client_address;
    bzero(&client_address, sizeof(client_address));
    socklen_t client_addrlen = sizeof(client_address);
    int connfd = accept(m_listen_sockfd, (struct sockaddr *) &client_address, &client_addrlen);
    if (connfd < 0) {
        std::cout << "accept() failed\n";
        return;
    }

    // 注册连接的过程在主线程中完成
    epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.data.fd = connfd;
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    int ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, connfd, &ev);
    if (ret < 0) {
        std::cout << "epoll_ctl() error\n";
        return;
    }

    std::cout << "register a new client\n";

    // 向连接池加入新的连接，在这里需要在子线程中完成
    m_producerThreads->enqueue([this, connfd, client_address]() {
        std::unique_lock<std::mutex> lock(m_mtxPool);
        m_condPool.wait(lock, [this]() -> bool {
            return !m_bPoolBlock && m_iOpeNum <= 0;
        });
        m_bPoolBlock = true;  // 添加锁住标记
        m_connectionPool.emplace(clientConnection(connfd, client_address));
        m_bPoolBlock = false; // 恢复标记
        m_condWrite.notify_all();
        m_condPool.notify_all();
    });
}

void MyServer::existConnection(int index) {

    // 客户端主动断开连接，这个一定要在前边，否则无法检测到
    // 连接的处理在主线程中完成
    if (m_epollEvents[index].events & EPOLLRDHUP) {

        // 取消注册的过程在主线程完成
        int fd = m_epollEvents[index].data.fd;
        if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
            std::cout << "delete client error\n";
            return;
        }
        std::cout << "delete a connected client\n";

        // 取消在连接池的连接在子线程中完成
        m_producerThreads->enqueue([this, fd]() -> bool {
            std::unique_lock<std::mutex> lock(m_mtxPool);
            m_condPool.wait(lock, [this]() -> bool {
                return !m_bPoolBlock && m_iOpeNum <= 0;
            });
            m_bPoolBlock = true;  // 加锁标记
            sockaddr_in addr;
            bzero(&addr, sizeof(addr));
            m_connectionPool.erase(clientConnection(fd, addr));
            m_bPoolBlock = false; // 解锁标记
            m_condWrite.notify_all();
            m_condPool.notify_all();
        });

    } else if (m_epollEvents[index].events & EPOLLIN) {  // 客户端发来消息

        std::cout << "receive msg\n";

        int fd = m_epollEvents[index].data.fd;

        // 接受消息并通知给写文件线程在生产者子线程中完成
        m_producerThreads->enqueue([this, fd]() {

            std::unique_lock<std::mutex> lock(m_mtxPool);
            m_condWrite.wait(lock, [this]() -> bool {   // 没有被全局封锁
                return !m_bPoolBlock;
            });
            lock.unlock();
            ++m_iOpeNum;
            sockaddr_in addr;
            bzero(&addr, sizeof(addr));
            auto it = m_connectionPool.find(clientConnection(fd, addr));
            if (it == m_connectionPool.end()) {
                return;
            }

            char buf[MAX_CHAR_BUFFER];
            memset(buf, 0, MAX_CHAR_BUFFER);
            it->clientMutex->lock();  // 接收数据要加锁
            std::cout << "produce sub thread deal msg\n";
            while (recv(fd, buf, MAX_CHAR_BUFFER, MSG_DONTWAIT) > 0) {
                *(it->clientMessage) += std::string(buf);
                std::cout << "msg = " << *(it->clientMessage) << std::endl;
                memset(buf, 0, MAX_CHAR_BUFFER);
            }
            it->clientMutex->unlock();
            m_qMsgQue.push(fd);
            m_condWrite.notify_all();
            --m_iOpeNum;
        });
    }
}

int MyServer::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
