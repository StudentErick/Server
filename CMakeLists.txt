cmake_minimum_required(VERSION 3.13)
project(Server)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS -pthread)

add_executable(Server main.cpp user_data.h ThreadPool.h ThreadSafeQueue.h TcpSocket.h TcpSocket.cpp TcpServer.h TcpServer.cpp MyServer.h MyServer.cpp)