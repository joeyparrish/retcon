/*
 * This file is part of RetCon: Retro Console Controller.
 *
 * RetCon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RetCon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RetCon.  If not, see <http://www.gnu.org/licenses/>.
 */

// This application runs on a Raspberry Pi.  It reads from a paired PS3
// controller and writes to the GPIO pins.  The GPIO pins drive an external
// circuit which interfaces to the game console.

#include <fcntl.h>
#include <linux/joystick.h>
#include <pigpio.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <map>
#include <vector>

const std::vector<int> harmless_signals = {
  SIGCONT, SIGTSTP, SIGTTIN, SIGTTOU
};

class Output {
 public:
  typedef enum {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    A,
    B,
    C,
    START,
  } Button;

  typedef enum {
    P1 = 0,
    P2,
  } Player;

  static void init() { instance_ = new Output(); }
  static inline Output* instance() { return instance_; }

  inline void write(Player player, Button b, bool on) {
    if (on) fprintf(stderr, "P%d button %d\n", player + 1, b);
    gpioWrite(pin_maps_[player].at(b), on);
  }

 private:
  Output();

  static Output* instance_;

  const std::map<Button, int> pin_maps_[2];
};

// static
Output* Output::instance_ = NULL;

Output::Output()
    : pin_maps_({
      {  // P1
        { UP, 14 },
        { DOWN, 24 },
        { LEFT, 23 },
        { RIGHT, 18 },
        { A, 7 },
        { B, 8 },
        { C, 25 },
        { START, 15 },
      },
      {  // P2
        { UP, 9 },
        { DOWN, 17 },
        { LEFT, 27 },
        { RIGHT, 22 },
        { A, 2 },
        { B, 3 },
        { C, 4 },
        { START, 10 },
      },
    }) {
  if (gpioInitialise() == PI_INIT_FAILED) {
    fprintf(stderr, "GPIO init failed.\n");
    exit(1);
  }

  for (auto& sig : harmless_signals) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = SIG_DFL;
    if (sigaction(sig, &action, NULL) != 0) {
      fprintf(stderr, "Signal handler %d could not be installed.\n", sig);
      exit(1);
    }
  }

  for (auto& map : pin_maps_) {
    for (auto& kv : map) {
      gpioSetMode(kv.second, PI_OUTPUT);
      gpioWrite(kv.second, 0);
    }
  }
}

class Input {
 public:
  Input(int fd);
  virtual ~Input() {}

  static void poll();

 protected:
  virtual bool connect() = 0;
  virtual bool read() = 0;
  void make_fd_nonblocking();

  int fd_;

  static std::vector<Input*> inputs_;
};

// static
std::vector<Input*> Input::inputs_;

Input::Input(int fd) : fd_(fd) {
  inputs_.push_back(this);
}

void Input::make_fd_nonblocking() {
  int flags = fcntl(fd_, F_GETFL, 0);
  fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
}

// static
void Input::poll() {
  while (true) {
    int max_fd = -1;
    int num_fds = 0;
    fd_set fds;
    FD_ZERO(&fds);

    for (auto& input : inputs_) {
      input->connect();
      if (input->fd_ < 0) continue;
      max_fd = std::max(max_fd, input->fd_);
      ++num_fds;
      FD_SET(input->fd_, &fds);
    }

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int rv = select(max_fd + 1, &fds, NULL, NULL, &tv);
    //fprintf(stderr, "Waited for %d fds, %d are ready\n", num_fds, rv);
    if (rv > 0) {
      for (auto& input : inputs_) {
        if (input->fd_ < 0) continue;
        if (FD_ISSET(input->fd_, &fds)) {
          while (input->read()) {}
        }
      }
    }
  }
}

class PS3 : public Input {
 public:
  PS3(Output::Player player, const char* path);
  virtual ~PS3() {}

 protected:
  virtual bool connect() override final;
  virtual bool read() override final;

 private:
  typedef enum {
    SELECT,
    L3,
    R3,
    START,
    UP,
    RIGHT,
    DOWN,
    LEFT,
    L2,
    R2,
    L1,
    R1,
    TRIANGLE,
    CIRCLE,
    X,
    SQUARE,
    PS,
  } Button;

  typedef enum {
    LEFT_ANALOG,
    RIGHT_ANALOG,
  } Stick;

  const Output::Player player_;
  const char* path_;
  const std::map<int, Output::Button> button_map_;

  typedef std::pair<Output::Button, Output::Button> AxisOutputs;
  const std::map<int, std::pair<AxisOutputs, AxisOutputs>> stick_map_;
};

PS3::PS3(Output::Player player, const char* path)
    : Input(-1),
      player_(player),
      path_(path),
      button_map_({
        { UP, Output::UP },
        { RIGHT, Output::RIGHT },
        { DOWN, Output::DOWN },
        { LEFT, Output::LEFT },
        { SQUARE, Output::A },
        { X, Output::B },
        { CIRCLE, Output::C },
        { R2, Output::C },
        { START, Output::START },
        { TRIANGLE, Output::START },
      }),
      stick_map_({
        { LEFT_ANALOG, {
          {Output::UP, Output::DOWN},
          {Output::LEFT, Output::RIGHT},
        } },
      }) {}

bool PS3::connect() {
  if (fd_ >= 0) {
    // already connected.
    return true;
  }

  // try to connect
  fd_ = open(path_, O_RDONLY);
  if (fd_ < 0) {
    // failed to connect
    return false;
  }

  // connected
  fprintf(stderr, "Connected: %s\n", path_);
  make_fd_nonblocking();
  return true;
}

bool PS3::read() {
  js_event event;
  int bytes = ::read(fd_, &event, sizeof(event));
  if (bytes != sizeof(event)) {
    if (errno != EAGAIN) {
      fprintf(stderr, "Failed to read: %s\n", path_);
      close(fd_);
      fd_ = -1;
    }
    return false;
  }

  Output* output = Output::instance();

  if (event.type & JS_EVENT_BUTTON) {
    //fprintf(stderr, "number: 0x%02x, value: 0x%04x\n", event.number, event.value);
    auto it = button_map_.find(event.number);
    if (it != button_map_.end()) {
      output->write(player_, it->second, event.value);
    }
  }
  if (event.type & JS_EVENT_AXIS) {
    //fprintf(stderr, "number: 0x%02x, value: %d\n", event.number, event.value);
    int stick_number = event.number >> 1;
    int axis_number = event.number & 1;
    auto it = stick_map_.find(stick_number);
    if (it != stick_map_.end()) {
      auto axes_outputs = it->second;
      auto axis_outputs = axis_number ? axes_outputs.first : axes_outputs.second;

      if (event.value < -20000) {
        output->write(player_, axis_outputs.first, true);
        output->write(player_, axis_outputs.second, false);
      } else if (event.value > 20000) {
        output->write(player_, axis_outputs.first, false);
        output->write(player_, axis_outputs.second, true);
      } else {
        output->write(player_, axis_outputs.first, false);
        output->write(player_, axis_outputs.second, false);
      }
    }
  }
  return true;
}

class Keyboard : public Input {
 public:
  Keyboard();
  virtual ~Keyboard() {}

 protected:
  virtual bool connect() override final;
  virtual bool read() override final;

 private:
  static void restore_echo();
  static void disable_echo();
  static int orig_lflag_;

  const std::map<int, Output::Button> button_map_;
};

// static
int Keyboard::orig_lflag_;

Keyboard::Keyboard()
    : Input(STDIN_FILENO),
      button_map_({
        { 'u', Output::UP },
        { 'd', Output::DOWN },
        { 'l', Output::LEFT },
        { 'r', Output::RIGHT },
        { 'a', Output::A },
        { 'b', Output::B },
        { 'c', Output::C },
        { 's', Output::START },
      }) {
  make_fd_nonblocking();
  disable_echo();
  atexit(restore_echo);
}

bool Keyboard::connect() {
  return true;
}

bool Keyboard::read() {
  char c;
  int bytes = ::read(STDIN_FILENO, &c, 1);
  if (bytes != 1) {
    return false;
  }

  char lower_c = c | 0x20;
  bool is_lower = lower_c == c;
  Output::Player player = is_lower ? Output::P1 : Output::P2;
  Output* output = Output::instance();

  auto it = button_map_.find(lower_c);
  if (it != button_map_.end()) {
    //fprintf(stderr, "'%c'\n", c);
    output->write(player, it->second, 1);
    usleep(100000);
    output->write(player, it->second, 0);
  }
  return true;
}

// static
void Keyboard::restore_echo() {
  fprintf(stderr, "Restoring echo.\n");
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_lflag = orig_lflag_;
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

// static
void Keyboard::disable_echo() {
  fprintf(stderr, "Disabling echo.\n");
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);
  orig_lflag_ = t.c_lflag;
  t.c_lflag &= ~(ICANON|ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

int main() {
  Output::init();
  Keyboard keyboard;
  PS3 ps3_p1(Output::P1, "/dev/input/js0");
  PS3 ps3_p2(Output::P2, "/dev/input/js1");
  Input::poll();  // never returns
  return 0;
}
