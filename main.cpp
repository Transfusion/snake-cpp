#include <ncurses.h>

#include <chrono>
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

void init_color() {
  start_color();
  init_pair(1, COLOR_GREEN, COLOR_RED);
  init_pair(2, COLOR_WHITE, COLOR_WHITE);
}

vector<pair<int, int>> DIRS = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};

deque<pair<int, int>> snake;
unordered_map<int, unordered_map<int, bool>> visited;

int apple_c, apple_r;
int UP = 0, RIGHT = 1, DOWN = 2, LEFT = 3;
int CUR_DIR = UP;  // initialize to going up

int score = 0, high_score = 0;
double speed = 1.0;
bool paused = false, terminated = false;

auto last_tick = chrono::steady_clock::now();

string get_status() {
  stringstream buf;
  buf << "Score: " << score << " High Score: " << high_score
      << " Speed: " << speed << " Dir: " << CUR_DIR << (paused ? " PAUSED" : "")
      << "       ";  // hack
  return buf.str();
}

int main() {
  srand(time(nullptr));
  WINDOW *w = initscr();

  int WORLD_WIDTH = min(60, COLS);
  int WORLD_HEIGHT = min(30 - 1, LINES - 1);  // reserve the last line for the score and paused status

  int offsetx = (COLS - WORLD_WIDTH) / 2;
  int offsety = (LINES - WORLD_HEIGHT) / 2;

  WINDOW *snake_win = newwin(WORLD_HEIGHT, WORLD_WIDTH, offsety, offsetx);
  nodelay(stdscr, TRUE);
  noecho();
  init_color();

  auto draw_apple = [snake_win](int r, int c) {
    wattron(snake_win, COLOR_PAIR(1));
    mvwaddch(snake_win, r, c, '@');
    wattroff(snake_win, COLOR_PAIR(1));
  };

  auto draw_snake_particle = [snake_win](int r, int c) {
    wattron(snake_win, COLOR_PAIR(2));
    mvwaddch(snake_win, r, c, ' ');
    wattroff(snake_win, COLOR_PAIR(2));
  };

  auto update_status = [offsety, WORLD_HEIGHT, offsetx]() {
    mvprintw(offsety + WORLD_HEIGHT, offsetx, get_status().c_str());
  };

  auto randomize_apple = [WORLD_WIDTH, WORLD_HEIGHT]() {
    apple_c = (rand() % (WORLD_WIDTH - 2) + 1);
    apple_r = (rand() % (WORLD_HEIGHT - 2) + 1);
  };

  auto init_game = [WORLD_WIDTH, WORLD_HEIGHT, offsetx, offsety, randomize_apple, draw_apple]() {
    score = 0, speed = 1.0;
    randomize_apple();
    int head_c = (WORLD_WIDTH / 2) + 0, head_r = (WORLD_HEIGHT / 2) + 0;
    deque<pair<int, int>>().swap(snake);  // clear q
    visited = unordered_map<int, unordered_map<int, bool>>();
    snake.emplace_front(make_pair(head_r, head_c));
    visited[head_r][head_c] = true;
  };

  // returns whether the move was successful/valid
  auto move_snake = [WORLD_HEIGHT, WORLD_WIDTH, randomize_apple]() {
    auto [dr, dc] = DIRS[CUR_DIR];
    auto [r, c] = snake.front();
    int nr = r + dr, nc = c + dc;
    // if oob
    if (nr == 0 || nc == 0 || nr == WORLD_HEIGHT - 1 || nc == WORLD_WIDTH - 1 || visited[nr][nc])
      return false;
    snake.emplace_front(make_pair(nr, nc));
    visited[nr][nc] = true;
    if (nr == apple_r && nc == apple_c) {
      score += 10;
      randomize_apple();
      high_score = max(score, high_score);
      if (score % 50 == 0) speed += 0.1;
    } else {
      auto [tailr, tailc] = snake.back();
      visited[tailr][tailc] = false;
      snake.pop_back();
    }
    return true;
  };

  auto render_snake_win = [draw_apple, draw_snake_particle, snake_win]() {
    box(snake_win, 0, 0);
    draw_apple(apple_r, apple_c);
    for (auto [r, c] : snake) draw_snake_particle(r, c);
  };

  auto poll_input = [update_status]() {
    char ch = getch();
    if (ch == 'i' && CUR_DIR != DOWN)
      CUR_DIR = UP;
    else if (ch == 'k' && CUR_DIR != UP)
      CUR_DIR = DOWN;
    else if (ch == 'j' && CUR_DIR != RIGHT)
      CUR_DIR = LEFT;
    else if (ch == 'l' && CUR_DIR != LEFT)
      CUR_DIR = RIGHT;
    else if (ch == 'p') {
      paused = !paused;
      update_status();
    } else if (ch == 'q')
      terminated = true;
  };

  init_game();
  update_status();

  timeout(10);  // because busy waiting sucks

  // main game loop
  while (!terminated) {
    auto cur_tick = chrono::steady_clock::now();
    auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(cur_tick - last_tick);
    int factor = (CUR_DIR == LEFT || CUR_DIR == RIGHT) ? 2 : 1;
    if (elapsed_time >= chrono::milliseconds(long((1 / speed) * 300 / factor)) && !paused) {
      update_status();
      werase(snake_win);
      render_snake_win();

      last_tick = cur_tick;
      wrefresh(snake_win);
      poll_input();
      if (!move_snake()) init_game();
    }
    if (paused && getch() == 'p') {
      paused = !paused;
      update_status();
    }
  }

  endwin();
  return 0;
}