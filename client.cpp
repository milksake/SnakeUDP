#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>

#define HOST_IP "127.0.0.1"
#define HOST_PORT "3490"
#define MAX_BUFFER_SIZE 1024

bool state = false; // playing?
bool exitt = false;
std::string lastMessage;

void rectangle(int y1, int x1, int y2, int x2)
{
    mvhline(y1, x1, 0, x2-x1);
    mvhline(y2, x1, 0, x2-x1);
    mvvline(y1, x1, 0, y2-y1);
    mvvline(y1, x2, 0, y2-y1);
    mvaddch(y1, x1, '*');
    mvaddch(y2, x1, '*');
    mvaddch(y1, x2, '*');
    mvaddch(y2, x2, '*');
}

void handleSend(int socket_fd, sockaddr* hostAddr, socklen_t addrLen, char userInitial) {
    char msgBuffer[MAX_BUFFER_SIZE];
    int keyPressed;
    while (!exitt) {
        keyPressed = getch();
        if (!state)
        {
            int keyPressed = getch();
            if (keyPressed == KEY_ENTER)
            {
                std::string mess = "I";
                mess.push_back(userInitial);
                sendto(socket_fd, mess.c_str(), mess.size(), 0, hostAddr, addrLen);
            }
            else if (keyPressed == KEY_BACKSPACE)
            {
                exitt = true;
                return;
            }
            continue;
        }
        std::string messageToSend;
        switch (keyPressed) {
            case KEY_UP:
                messageToSend = "Mu" + std::string(1, userInitial);
                break;
            case KEY_DOWN:
                messageToSend = "Md" + std::string(1, userInitial);
                break;
            case KEY_LEFT:
                messageToSend = "Ml" + std::string(1, userInitial);
                break;
            case KEY_RIGHT:
                messageToSend = "Mr" + std::string(1, userInitial);
                break;
            default:
                continue;
        }
        memset(msgBuffer, 0, MAX_BUFFER_SIZE);
        strncpy(msgBuffer, messageToSend.c_str(), MAX_BUFFER_SIZE);
        sendto(socket_fd, msgBuffer, MAX_BUFFER_SIZE, 0, hostAddr, addrLen);
    }
}

void handleReceive(int socket_fd, char userInitial) {
    char recvBuffer[MAX_BUFFER_SIZE];
    while (!exitt) {
        memset(recvBuffer, 0, MAX_BUFFER_SIZE);
        recvfrom(socket_fd, recvBuffer, MAX_BUFFER_SIZE, 0, nullptr, nullptr);
        switch (recvBuffer[0]) {
            case 'Y':
                state = true;
                break;
            case 'N':
                mvprintw(0, 0, "Initial already in use. Disconnecting...");
                refresh();
                sleep(2);
                endwin();
                close(socket_fd);
                exit(0);
                break;
            case 'W':
                if (recvBuffer[1] == userInitial) {
                    clear();
                    mvprintw(10, 10, " __        __         _       _   _             ");
                    mvprintw(11, 10, " \\ \\      / /__  _ __| | __ _| |_(_) ___  _ __  ");
                    mvprintw(12, 10, "  \\ \\ /\\ / / _ \\| '__| |/ _` | __| |/ _ \\| '_ \\ ");
                    mvprintw(13, 10, "   \\ V  V / (_) | |  | | (_| | |_| | (_) | | | |");
                    mvprintw(14, 10, "    \\_/\\_/ \\___/|_|  |_|\\__,_|\\__|_|\\___/|_| |_|");
                    refresh();
                    sleep(5);
                    lastMessage = "Press Enter to play again. Press Backspace to exit.";
                    state = false;
                }
                break;
            case 'L':
                if (recvBuffer[1] == userInitial) {
                    clear();
                    mvprintw(10, 10, "  _                   _               ");
                    mvprintw(11, 10, " | |    _____   _____| | ___  _ __ ___ ");
                    mvprintw(12, 10, " | |   / _ \\ \\ / / _ \\ |/ _ \\| '__/ _ \\");
                    mvprintw(13, 10, " | |__|  __/\\ V /  __/ | (_) | | |  __/");
                    mvprintw(14, 10, " |_____\\___| \\_/ \\___|_|\\___/|_|  \\___|");
                    refresh();
                    sleep(5);
                    lastMessage = "Press Enter to play again. Press Backspace to exit.";
                    state = false;
                }
                else
                {
                    lastMessage = "User: ";
                    lastMessage.push_back(recvBuffer[1]);
                    lastMessage += "died";
                }
            case 'm':
            {
                clear();  // Limpia la pantalla antes de imprimir el nuevo mapa
                rectangle(0, 0, 22, 42);
                std::string board;
                for (int i = 0; i < 20; i++)
                {
                    for (int j = 0; j < 40; j++)
                    {
                        board.push_back(recvBuffer[1 + i*40 + j]);
                    }
                    mvprintw(i+1, 1, "%s", board.c_str());  // Imprime el mapa recibido desde la posición (0, 0)
                    board.clear();
                }
                mvprintw(44, 1, lastMessage.c_str());
            }
                break;
            default:
                mvprintw(1, 0, "Unknown message: %s", recvBuffer);
                break;
        }
        refresh();  // Actualiza la ventana para mostrar los cambios
    }
}

int main() {
    int socket_fd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(HOST_IP, HOST_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    char username[MAX_BUFFER_SIZE];

    std::cout << "Enter your name: ";
    std::cin.getline(username, MAX_BUFFER_SIZE);
    char userInitial = username[0];

    initscr();            // Inicializa ncurses
    cbreak();             // Desactiva el buffering de línea
    noecho();             // No muestra las teclas presionadas
    keypad(stdscr, TRUE); // Habilita la captura de teclas especiales

    // Enviar mensaje de inicio al servidor
    char initMsg[3] = {'I', userInitial, '\0'};
    sendto(socket_fd, initMsg, sizeof(initMsg), 0, p->ai_addr, p->ai_addrlen);

    std::thread sendThread(handleSend, socket_fd, p->ai_addr, p->ai_addrlen, userInitial);
    std::thread receiveThread(handleReceive, socket_fd, userInitial);

    sendThread.join();
    receiveThread.join();

    endwin();             // Finaliza ncurses y restaura la configuración normal de la terminal
    close(socket_fd);
    return 0;
}
