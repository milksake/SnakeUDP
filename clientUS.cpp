#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <ncurses.h>

#define HOST_IP "127.0.0.1"
#define HOST_PORT 3490
#define MAX_BUFFER_SIZE 1024
#define BOARD_WIDTH 42
#define BOARD_HEIGHT 22

void handleSend(int socket_fd, sockaddr_in hostAddr, socklen_t addrLen, char userInitial) {
    char msgBuffer[MAX_BUFFER_SIZE];
    int keyPressed;
    while (true) {
        keyPressed = getch();
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
        sendto(socket_fd, msgBuffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&hostAddr, addrLen);
    }
}

bool firstPrint = true;
void handleReceive(int socket_fd, char userInitial) {
    char recvBuffer[MAX_BUFFER_SIZE];
    while (true) {
        memset(recvBuffer, 0, MAX_BUFFER_SIZE);
        recvfrom(socket_fd, recvBuffer, MAX_BUFFER_SIZE, 0, nullptr, nullptr);
        switch (recvBuffer[0]) {
            case 'Y':
                clear();  // Limpia la pantalla antes de imprimir el nuevo mapa
                mvprintw(0, 0, "%s", recvBuffer + 1);  // Imprime el mapa recibido desde la posición (0, 0)
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
                    endwin();
                    close(socket_fd);
                    exit(0);
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
                    endwin();
                    close(socket_fd);
                    exit(0);
                }
                break;
            case 'm':
                clear();  // Limpia la pantalla antes de imprimir el nuevo mapa
                // mvprintw(0, 0, "%s", recvBuffer + 1);  // Imprime el mapa recibido desde la posición (0, 0)
                for (int i = 0; i < BOARD_HEIGHT; ++i) {
                mvprintw(i, 0, "#");
                mvprintw(i, BOARD_WIDTH - 1, "#");

                if (i == 0 || i == BOARD_HEIGHT - 1) {
                    for (int j = 1; j < BOARD_WIDTH - 1; ++j) {
                        mvprintw(i, j, "#");
                    }
                } else {
                    if (firstPrint) {
                        mvprintw(i, 1, "%.*s", BOARD_WIDTH - 2, recvBuffer + 1 + (i - 1) * (BOARD_WIDTH - 2));
                    } else {
                        mvprintw(i, 1, "%.*s", BOARD_WIDTH - 2, recvBuffer + 1 + (i - 1) * BOARD_WIDTH - 2);
                    }
                }
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
    struct sockaddr_in hostAddr;
    socklen_t addrLen = sizeof(hostAddr);
    char username[MAX_BUFFER_SIZE];

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&hostAddr, 0, sizeof(hostAddr));
    hostAddr.sin_family = AF_INET;
    hostAddr.sin_port = htons(HOST_PORT);
    hostAddr.sin_addr.s_addr = inet_addr(HOST_IP);

    std::cout << "Enter your name: ";
    std::cin.getline(username, MAX_BUFFER_SIZE);
    char userInitial = username[0];

    initscr();            // Inicializa ncurses
    cbreak();             // Desactiva el buffering de línea
    noecho();             // No muestra las teclas presionadas
    keypad(stdscr, TRUE); // Habilita la captura de teclas especiales

    // Enviar mensaje de inicio al servidor
    char initMsg[3] = {'I', userInitial, '\0'};
    sendto(socket_fd, initMsg, sizeof(initMsg), 0, (struct sockaddr*)&hostAddr, addrLen);

    // Recibir respuesta del servidor
    char response[MAX_BUFFER_SIZE];
    recvfrom(socket_fd, response, MAX_BUFFER_SIZE, 0, nullptr, nullptr);
    if (response[0] == 'Y') {
        mvprintw(0, 0, "Connection successful. Game starting...");
        mvprintw(1, 0, "%s", response + 1);  // Imprime el mapa inicial
        refresh();
    } else if (response[0] == 'N') {
        mvprintw(0, 0, "Initial already in use. Disconnecting...");
        refresh();
        sleep(2);
        endwin();
        close(socket_fd);
        return 0;
    }

    std::thread sendThread(handleSend, socket_fd, hostAddr, addrLen, userInitial);
    std::thread receiveThread(handleReceive, socket_fd, userInitial);

    sendThread.join();
    receiveThread.join();

    endwin();             // Finaliza ncurses y restaura la configuración normal de la terminal
    close(socket_fd);
    return 0;
}
