#include "server.h"

UDPListener::UDPListener(const std::function<void(const int, const std::string&, sockaddr_storage, socklen_t)> &_func, const char* _port) :
    func(_func), keep(false)
{
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, _port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(201);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((socketFD = socket(p->ai_family, p->ai_socktype,
        p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }
        if (bind(socketFD, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketFD);
            perror("listener: bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        exit(202);
    }

    freeaddrinfo(servinfo);
}

void UDPListener::loop()
{
    while (keep)
    {
        const int maxbuff = 1000;
        char buff[maxbuff+1];
        struct sockaddr_storage addr;
        socklen_t addr_size = sizeof addr;

        int numbytes;

        addr_size = sizeof addr;
        if ((numbytes = recvfrom(socketFD, buff, maxbuff, 0, (struct sockaddr *)&addr, &addr_size)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        buff[numbytes] = '\0';
        std::string datum(buff, numbytes);
        
        std::thread th(func, socketFD, datum, addr, addr_size);
        th.detach();
    }
}

void UDPListener::run()
{
    if (keep == true)
        return;
    keep = true;

    std::thread th(&UDPListener::loop, this);
    th.detach();
}

void UDPListener::stop()
{
    keep = false;
}

bool UDPListener::isRunning()
{
    return keep;
}

void sendString(const int socket, const std::string& message, sockaddr_storage addr, socklen_t addr_size)
{
    sendto(socket, message.c_str(), message.size(), 0, (struct sockaddr *) &addr, addr_size);
}

/* signed main()
{
    bool keep = true;

    auto lamb = [&keep](const int socket, const std::string& datum, sockaddr_storage addr, socklen_t addr_size) {
        std::cout << datum << '\n';
        sendString(socket, "Hola", addr, addr_size);
        keep = false;
    };

    UDPListener listener(lamb, PORT);

    listener.run();

    while(keep) {}

    listener.stop();
} */
