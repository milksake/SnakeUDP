#include <unordered_map>
#include <vector>
#include <map>
#include <queue>
#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include "server.cpp"

#define PORT "3490"
#define BOARD_WIDTH 40
#define BOARD_HEIGHT 20

struct Snake {
  std::vector<std::pair<int, int>> body;
  char initial;
  char direction;
};

std::map<char, Snake> snakes;
char board[BOARD_HEIGHT][BOARD_WIDTH];
int scores[256] = {0}; // Scores for each initial

void initialize_board() {
  memset(board, ' ', sizeof(board));
}

void update_board() {
  memset(board, ' ', sizeof(board));
  for (const auto& pair : snakes) {
    const Snake& snake = pair.second;
    for (const auto &part : snake.body) {
      board[part.first][part.second] = snake.initial;
    }
  }
}

bool is_collision(const std::pair<int, int>& head, char initial) {
  return board[head.first][head.second] != ' ' && board[head.first][head.second] != initial;
}

void bfs_clear_snake(char initial) {
}

void move_snake(char initial, char direction) {
}

int socketFD;
std::unordered_map<char, std::function<void(const std::string&, sockaddr_storage, socklen_t)>> handlers;

void handle_initialization(const std::string& message, sockaddr_storage addr, socklen_t addr_size) {
  char initial = message[1];
  std::string response;
  if (snakes.find(initial) != snakes.end()) {
    response = "N";
  } else {
    /*inicializar snake*/
    response = "Y";
  }
  sendString(socketFD, response, addr, addr_size);
}

void handle_move(const std::string& message, sockaddr_storage addr, socklen_t addr_size) {
  char initial = message[2];
  char direction = message[1];
  move_snake(initial, direction);
  update_board();
  std::string response = "m";
  for (int i = 0; i < BOARD_HEIGHT; ++i) {
    for (int j = 0; j < BOARD_WIDTH; ++j) {
      response += board[i][j];
    }
  }
  sendString(socketFD, response, addr, addr_size);
}

void main_handler(const std::string& datum, sockaddr_storage addr, socklen_t addr_size) {
  char message_type = datum[0];
  if (handlers.find(message_type) != handlers.end()) {
    handlers[message_type](datum, addr, addr_size);
  }
}

signed main() {
  initialize_board();

  handlers['I'] = handle_initialization;
  handlers['M'] = handle_move;

  auto handler = [](const int socket, const std::string& datum, sockaddr_storage addr, socklen_t addr_size) {
    main_handler(datum, addr, addr_size);
    socketFD = socket;
  };

  UDPListener listener(handler, PORT);

  listener.run();

  while (true) {
  }

  listener.stop();
  std::this_thread::sleep_for(std::chrono::seconds(2));

  return 0;
}
