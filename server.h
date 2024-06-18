#include <functional>
#include <thread>
#include <string>
#include <vector>
#include <stdio.h>
#include <map>
#include <unordered_map>
#include <queue>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <iostream>

class UDPListener
{
    int socketFD;
    bool keep;
    std::function<void(const int, const std::string&, sockaddr_storage, socklen_t)> func;

    void loop();

public:
    UDPListener(const std::function<void(const int sock, const std::string& datum, sockaddr_storage addr, socklen_t addr_size)>& _func, const char* _port);

    void run();
    void stop();
    bool isRunning();
    int getSocketFD() const { return socketFD; }
};

void sendString(const int socket, const std::string& message, sockaddr_storage addr, socklen_t addr_size);
