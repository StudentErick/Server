//
// Created by erick on 12/27/18.
//

#ifndef SERVER_MYSERVER_H
#define SERVER_MYSERVER_H

#include "TcpServer.h"
#include "ThreadPool.h"
#include "ThreadSafeQueue.h"

#include <queue>
#include <condition_variable>
#include <iostream>
#include <fstream>

class MyServer : public TcpServer {
public:
    // 读取数据、写入数据、等待队列长度
    explicit MyServer(int _proNums, int _conNums, int _maxWaiters = 5);

    ~MyServer();

    void newConnection() override;

    void existConnection(int index) override;

    int setnonblocking(int fd);

private:
    std::unique_ptr<ThreadPool> m_producerThreads;
    std::unique_ptr<ThreadPool> m_consumerThreads;

    ConnectionPool m_connectionPool;  // 连接池
    ThreadSafeQueue<int> m_qMsgQue;   // 消息队列，精确通知写线程

    std::atomic_bool m_bStop;
    std::atomic_bool m_bWriteBlock;   // 是否可以写入文件
    std::atomic_int m_iOpeNum;        // 正在读取或者写入连接的操作个数
    std::atomic_bool m_bPoolBlock;    // 整个连接池是否被锁住

    std::mutex m_mtxWrite;  // 写文件的锁，同时只能有一个线程写文件
    std::mutex m_mtxPool;   // 连接池的总锁，用于插入和删除的互斥

    std::condition_variable m_condWrite;  // 唤醒写连接池的操作
    std::condition_variable m_condPool;   // 唤醒插入或者删除文件的操作

    std::unique_ptr<std::fstream> m_fileStream;       // 文件流

    std::atomic_int cnt;

};


#endif //SERVER_MYSERVER_H
