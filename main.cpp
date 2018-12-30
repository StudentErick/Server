#include <iostream>
#include "MyServer.h"
#include <string.h>

int main(int argc, char *argv[]) {
    MyServer myServer(4, 1, 1000);
    if (!myServer.listen(8001)) {
        std::cout << "server listen error\n";
        return 0;
    }

    if (!myServer.startService()) {
        std::cout << "start service error\n";
        return 0;
    }

    std::cout << "service end\n";

    return 0;
}