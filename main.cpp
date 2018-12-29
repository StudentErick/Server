#include <iostream>
#include "MyServer.h"
#include <string.h>

int main(int argc, char *argv[]) {
    MyServer myServer(4, 1);
    if (!myServer.listen(8001)) {
        std::cout << "server listen error\n";
        return 0;
    }

    if (!myServer.startService()) {
        std::cout << "start service error\n";
        return 0;
    }

    return 0;
}