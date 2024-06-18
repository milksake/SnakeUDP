#include "server.h"

#define BOARD_WIDTH 40
#define BOARD_HEIGHT 20

struct Snake {
  std::vector<std::pair<int, int>> body;
  char initial;
  char direction;
};

std::map<char, Snake> snakes;
char board[BOARD_HEIGHT][BOARD_WIDTH];
int scores[256] = {0};

void initialize_board() {
  memset(board, ' ', sizeof(board));
}

void update_board() {
  memset(board, ' ', sizeof(board));
  for (const auto& pair : snakes) {
    const Snake& snake = pair.second;
    for (const auto &part : snake.body) {
      std::cout << part.first << " " << part.second << " ";
      board[part.first][part.second] = snake.initial;
      std::cout << snake.initial << " ";
    }
    std::cout << std::endl;
  }
}

bool is_collision(const std::pair<int, int>& head, char initial) {
  return board[head.first][head.second] != ' ' && board[head.first][head.second] != initial;
}

void bfs_clear_snake(char initial) {
  std::queue<std::pair<int, int>> q;
  for (int i = 0; i < BOARD_HEIGHT; ++i) {
    for (int j = 0; j < BOARD_WIDTH; ++j) {
      if (board[i][j] == initial) {
        q.push({i, j});
      }
    }
  }

  while (!q.empty()) {
      auto [x, y] = q.front(); q.pop();
      board[x][y] = ' ';
  }
}


void move_snake(int socket, char initial, char direction, sockaddr_storage addr, socklen_t addr_size) {
  if (snakes.find(initial) == snakes.end()) return;
  Snake& snake = snakes[initial];
  auto head = snake.body.front();
  std::pair<int, int>& new_head = head;

  std::cout << "direction: " << direction << std::endl;

  switch (direction) {
  case 'u':
    std::cout << "moving UP" << std::endl;
    new_head.first -= 1;
    break;
  case 'd':
    std::cout << "moving DOWN" << std::endl;
    new_head.first += 1;
    break;
  case 'l':
    std::cout << "moving LEFT" << std::endl;
    new_head.second -= 1;
    break;
  case 'r':
    std::cout << "moving RIGHT" << std::endl;
    new_head.second += 1;
    break;
  }

  if (is_collision(new_head, initial)) {
    bfs_clear_snake(initial);
    snakes.erase(initial);
    std::string message = "L";
    message += initial;
    sendString(socket, message, addr, addr_size);
    return;
  }

  snake.body.insert(snake.body.begin(), new_head);
  if (snake.body.size() > 5) {
    snake.body.pop_back();
  }
  snake.direction = direction;
}

std::unordered_map<char, std::function<void(int socket, const std::string&, sockaddr_storage, socklen_t)>> handlers;

void handle_initialization(int socket, const std::string& message, sockaddr_storage addr, socklen_t addr_size) {
  char initial = message[1];
  std::string response;
  if (snakes.find(initial) != snakes.end()) {
    response = "N";
  } else {
    std::vector<std::pair<int, int>> body(5, {BOARD_HEIGHT / 2, BOARD_WIDTH / 2});
    for (int i = 1; i < 5; ++i) {
      body[i].second -= i;
    }
    snakes[initial] = Snake{body, initial, 'U'};
    response = "Y";
  }
  sendString(socket, response, addr, addr_size);
}

void handle_move(int socket, const std::string& message, sockaddr_storage addr, socklen_t addr_size) {
  char initial = message[2];
  char direction = message[1];
  move_snake(socket, initial, direction, addr, addr_size);
  update_board();
  std::string response = "m";
  for (int i = 0; i < BOARD_HEIGHT; ++i) {
    for (int j = 0; j < BOARD_WIDTH; ++j) {
      response += board[i][j];
    }
  }
  sendString(socket, response, addr, addr_size);
}

void main_handler(int socket, const std::string& datum, sockaddr_storage addr, socklen_t addr_size) {
  std::cout << datum << std::endl;
  char message_type = datum[0];
  if (handlers.find(message_type) != handlers.end()) {
    handlers[message_type](socket, datum, addr, addr_size);
  }
}

int main() {
  initialize_board();

  handlers['I'] = handle_initialization;
  handlers['M'] = handle_move;

  auto handler = [](const int socket, const std::string& datum, sockaddr_storage addr, socklen_t addr_size) {
    main_handler(socket, datum, addr, addr_size);
  };

  UDPListener listener(handler, PORT);

  listener.run();

  while (true) {
  }

  listener.stop();
  std::this_thread::sleep_for(std::chrono::seconds(2));

  return 0;
}
