#include "server.h"
#include <cmath>
#include <typeinfo>
#include <mutex>

#define PI 3.1416

#define BOARD_WIDTH 40
#define BOARD_HEIGHT 20
#define WINNER_POINTS 100
#define TIME 160


std::vector<std::pair<std::string, std::pair<sockaddr_storage, socklen_t>>> toProcess;
int currx = 0;

struct Snake {
  std::vector<std::pair<int, int>> body;
  char initial;
  char direction;
  int points = 0;
};

struct Bullet
{
  int y;
  int x;
  char direction;
};

std::vector<Bullet> bullets;
std::map<char, Snake> snakes;
char OGBoard[BOARD_HEIGHT][BOARD_WIDTH];
char board[BOARD_HEIGHT][BOARD_WIDTH];
std::unordered_map<char, sockaddr_storage> clients;
std::unordered_map<char, socklen_t> clients_sizes;

std::mutex mt;

char& getMap(int y, int x)
{
  return OGBoard[y][(x + currx) % BOARD_WIDTH];
}

void printBoard(char b[BOARD_HEIGHT][BOARD_WIDTH])
{
  for (int i = 0; i < BOARD_HEIGHT; i++)
  {
    for (int j = 0; j < BOARD_WIDTH; j++)
      std::cout << b[i][j];
    std::cout << '\n';
  }
}

void initialize_board() {
  memset(OGBoard, ' ', sizeof(OGBoard));

  double a = 5.0;
  double b = 10.0;
  int offset = 12;
  for (int i = 0; i < BOARD_WIDTH; i++)
  {
    double y = a/2.0f*std::sin(PI/b*i) + a/2.0f;
    OGBoard[(int)y][i] = '#';
    OGBoard[BOARD_HEIGHT - 1 - (int)y][(i + offset) % BOARD_WIDTH] = '#';
  }

  // printBoard(OGBoard);

}

void update_board() {
  // std::cout << "UPDATE BEGIN\n";
  for (int i = 0; i < BOARD_HEIGHT; i++)
  {
    for (int j = 0; j < BOARD_WIDTH; j++)
      board[i][j] = getMap(i, j);
  }
  for (const auto& pair : snakes) {
    const Snake& snake = pair.second;
    for (const auto &part : snake.body) {
      // std::cout << part.first << " " << part.second << " ";
      if (part.first < 0 || part.first >= BOARD_HEIGHT || part.second < 0 || part.second >= BOARD_WIDTH)
        continue;
      board[part.first][part.second] = snake.initial;
      // std::cout << snake.initial << " ";
    }
    // std::cout << std::endl;
  }
  for (const auto& b : bullets)
  {
    board[b.y][b.x] = '@';
  }
  // std::cout << "UPDATE FIN\n";
}

bool is_collision(const std::pair<int, int>& head, char initial) {
    return (board[head.first][head.second] != ' ' && board[head.first][head.second] != initial);
}

void bfs_clear_snake(char initial) {
  std::queue<std::pair<int, int>> q;
  for (int i = 0; i < BOARD_HEIGHT; ++i) {
    for (int j = 0; j < BOARD_WIDTH; ++j) {
      if (getMap(i,j) == initial) {
        q.push({i, j});
      }
    }
  }

  while (!q.empty()) {
      auto [x, y] = q.front(); q.pop();
      getMap(x,y) = ' ';
  }
}

void broadcast(int socket, const std::string& message) {
  for (const auto& client : clients) {
    sendString(socket, message, client.second, clients_sizes[client.first]);
  }
}

void move_snake(int socket, char initial, char direction, sockaddr_storage addr, socklen_t addr_size) {
  if (snakes.find(initial) == snakes.end()) return;
  Snake& snake = snakes[initial];
  auto head = snake.body.front();
  std::vector<std::pair<int, int>> new_body = snake.body;

  bool type;
  int diff;
  switch (direction) {
  case 'u':
    for (int i = 0; i < 5; i++)
      new_body[i].first -= 1;
    type = 0;
    diff = -1;
    break;
  case 'd':
    for (int i = 0; i < 5; i++)
      new_body[i].first += 1;
    type = 0;
    diff = 1;
    break;
  case 'l':
    for (int i = 0; i < 5; i++)
      new_body[i].second -= 1;
    type = 1;
    diff = -1;
    break;
  case 'r':
    for (int i = 0; i < 5; i++)
      new_body[i].second += 1;
    type = 1;
    diff = 1;
    break;
  }

  auto new_head = new_body.front();
  if (new_head.first < 0 || new_head.first >= BOARD_HEIGHT || new_head.second < 0 || new_head.second >= BOARD_WIDTH || getMap(new_head.first, new_head.second) == '#')
  {
    // bfs_clear_snake(initial);
    snakes.erase(initial);
    std::string message = "L";
    message += initial;
    sendString(socket, message, addr, addr_size);
    std::cout << "OUT OF BOUNDS" << std::endl;
  }
  else
  {
    int i = 0;
    for (; i < 5; i++)
    {
      if (is_collision(new_body[i], initial))
        break;
    }
    if (i < 5) {
      new_head = new_body[i];
      char del = board[new_head.first][new_head.second];
      // bfs_clear_snake(del);
      snakes.erase(del);
      std::string message = "L";
      message += del;
      broadcast(socket, message);
      std::cout << "CHOQUE" << std::endl;

      if(++snake.points == WINNER_POINTS){
        message = "W";
        message += initial;
        broadcast(socket, message);
      }

      return;
    }

    // snake.body.insert(snake.body.begin(), new_head);
    // if (snake.body.size() > 5) {
    //   snake.body.pop_back();
    // }
    // snake.direction = direction;

    for (auto& seg : snake.body)
    {
      if (type)
      {
        seg.second += diff;
      }
      else
      {
        seg.first += diff;
      }
    }
  }
}

std::unordered_map<char, std::function<void(int socket, const std::string&, sockaddr_storage, socklen_t)>> handlers;

void handle_initialization(int socket, const std::string& message, sockaddr_storage addr, socklen_t addr_size) {
  char initial = message[1];
  std::string response;
  bool ini = true;
  if (snakes.find(initial) != snakes.end()) {
    response = "N";
    ini = false;
  } else {
    std::vector<std::pair<int, int>> body(5);
    body[0] = {BOARD_HEIGHT / 2, BOARD_WIDTH / 2};
    body[1] = {BOARD_HEIGHT / 2 + 1, BOARD_WIDTH / 2};
    body[2] = {BOARD_HEIGHT / 2 - 1, BOARD_WIDTH / 2};
    body[3] = {BOARD_HEIGHT / 2, BOARD_WIDTH / 2 + 1};
    body[4] = {BOARD_HEIGHT / 2, BOARD_WIDTH / 2 - 1};
    auto& uu = snakes[initial];
    uu = Snake{body, initial, 'U'};
    clients[initial] = addr;
    clients_sizes[initial] = addr_size;
    response = "Y";
  }
  sendString(socket, response, addr, addr_size);
  if (ini)
  {
    update_board();
  }
}

void handle_move(int socket, const std::string& message, sockaddr_storage addr, socklen_t addr_size) {
  char initial = message[2];
  char direction = message[1];
  move_snake(socket, initial, direction, addr, addr_size);
  update_board();
}

void handle_shoot(int socket, const std::string& message, sockaddr_storage addr, socklen_t addr_size)
{
  char initial = message[1];
  if (snakes.find(initial) == snakes.end()) return;
  Snake& snake = snakes[initial];
  auto head = snake.body.front();

  bullets.push_back(Bullet{head.first - 2, head.second, 'U'});
  bullets.push_back(Bullet{head.first + 2, head.second, 'D'});
  bullets.push_back(Bullet{head.first, head.second - 2, 'L'});
  bullets.push_back(Bullet{head.first, head.second + 2, 'R'});
  update_board();
}

void main_handler(int socket, const std::string& datum, sockaddr_storage addr, socklen_t addr_size) {
  std::cout << datum << std::endl;
  char message_type = datum[0];
  if (handlers.find(message_type) != handlers.end()) {
    toProcess.push_back({datum, {addr, addr_size}});
    // handlers[message_type](socket, datum, addr, addr_size);
    // std::cout << "PUSHED\n";
  }
}

void update(int socket)
{
  currx += 1;

  std::vector<char> initials_delete;

  for (const auto& pair : snakes) {
    const Snake& snake = pair.second;
    auto part = snake.body.front();
    char initial = snake.initial;
    if (getMap(part.first, part.second) == '#')
    {
      // bfs_clear_snake(initial);
      // snakes.erase(initial);
      initials_delete.push_back(initial);
      std::string message = "L";
      message += initial;
      sendString(socket, message, clients[initial], clients_sizes[initial]);
      std::cout << "OUT OF BOUNDS" << std::endl;
    }
  }

  for (auto c : initials_delete)
  {
    snakes.erase(c);
  }

  update_board();

  for (int i = 0; i < bullets.size(); i++)
  {
    int newX = bullets[i].x, newY = bullets[i].y;
    // std::cout << bullets[i].direction << '\n';
    switch (bullets[i].direction)
    {
      case 'U':
      {
        newY -= 1;
      }
      break;
      case 'D':
      {
        newY += 1;
      }
      break;
      case 'L':
      {
        newX -= 1;
      }
      break;
      case 'R':
      {
        newX += 1;
      }
      break;
    }
    std::cout << bullets[i].x << ' ' << bullets[i].y << ' ' << bullets[i].direction << '\n';
    std::cout << '\t' << newX << ' ' << newY << ' ' << bullets[i].direction << '\n';

    if (newX < 0 || newX >= BOARD_WIDTH || newY < 0 || newY >= BOARD_HEIGHT)
    {
      bullets.erase(bullets.begin() + i);
      i--;
      continue;
    }

    if (board[newY][newX] != '#' && board[newY][newX] != ' ' && board[newY][newX] != '@')
    {
      char initial = board[newY][newX];
      snakes.erase(initial);
      std::string message = "L";
      message += initial;
      sendString(socket, message, clients[initial], clients_sizes[initial]);
      std::cout << "Killed " << initial << std::endl;
    }

    bullets[i].x = newX;
    bullets[i].y = newY;

    update_board();
  }

  for (auto& mess : toProcess)
  {
    std::cout << mess.first << '\n';
    char message_type = mess.first[0];
    handlers[message_type](socket, mess.first, mess.second.first, mess.second.second);
    std::cout << mess.first << '\n';
  }

  toProcess.clear();

  std::string response = "m";
  for (int i = 0; i < BOARD_HEIGHT; ++i) {
    for (int j = 0; j < BOARD_WIDTH; ++j) {
      response += board[i][j];
    }
  }
  broadcast(socket, response);
}

void ticker(int socket)
{
  while(true)
  {
    mt.lock();
    update(socket);
    mt.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(TIME));
  }
}

int main() {
  initialize_board();

  handlers['I'] = handle_initialization;
  handlers['M'] = handle_move;
  handlers['S'] = handle_shoot;

  UDPListener listener(&main_handler, PORT);

  listener.run();

  std::thread th(&ticker, listener.getSocketFD());

  while (true) {
  }

  listener.stop();
  std::this_thread::sleep_for(std::chrono::seconds(2));

  return 0;
}
