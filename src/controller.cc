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

#include <errno.h>
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

// pigpio installs signal handlers that shut down the application on a large
// number of signals.  These are signals that are harmless and are used to
// suspend and background applications.  For each of these, we will override
// the pigpio handler so that these signals can be used normally.
const std::vector<int> harmless_signals = {
  SIGCONT, SIGTSTP, SIGTTIN, SIGTTOU
};

// Output represents the application's output to the external circuit through
// the Raspberry Pi's GPIO pins.
class Output {
 public:
  // These represent Sega Genesis buttons.
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

  // Player 1, player 2
  typedef enum {
    P1 = 1,
    P2 = 2,
  } Player;

  static void init() { instance_ = new Output(); }
  static inline Output* instance() { return instance_; }

  // Update the relevant GPIO pin state.
  inline void write(Player player, Button b, bool on) {
    if (on) fprintf(stderr, "P%d button %d\n", player, b);
    gpioWrite(pin_maps_[player].at(b), on);
  }

 private:
  Output();

  static Output* instance_;

  // Maps buttons for each player to specific GPIO pins.
  const std::map<Button, int> pin_maps_[3];
};

// static
Output* Output::instance_ = NULL;

Output::Output()
    : pin_maps_({
      // slot zero is unused, so that the P1 and P2 enums have more natural
      // values.
      {},

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
  // Init GPIO.
  if (gpioInitialise() == PI_INIT_FAILED) {
    fprintf(stderr, "GPIO init failed.\n");
    exit(1);
  }

  // Ignore harmless signals, on which pigpio would otherwise exit.
  for (auto& sig : harmless_signals) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = SIG_DFL;
    if (sigaction(sig, &action, NULL) != 0) {
      fprintf(stderr, "Signal handler %d could not be installed.\n", sig);
      exit(1);
    }
  }

  // Configure the output pins and clear them.
  for (auto& map : pin_maps_) {
    for (auto& kv : map) {
      gpioSetMode(kv.second, PI_OUTPUT);
      gpioWrite(kv.second, 0);
    }
  }
}

// Input is an abstract base which represents input methods.
class Input {
 public:
  Input(int fd);
  virtual ~Input() {}

  // Polls all inputs.  Never returns.
  static void poll();

 protected:
  // Connect to the input, if not already connected.
  virtual bool connect() = 0;

  // Read button presses from the input.
  virtual bool read() = 0;

  // Make fd_ non-blocking.
  void make_fd_nonblocking();

  // A file descriptor representing the input.
  int fd_;

  // A vector of all existing input objects.
  static std::vector<Input*> inputs_;
};

// static
std::vector<Input*> Input::inputs_;

Input::Input(int fd) : fd_(fd) {
  // Register this input instance with the static list.
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

    // Collect a set of file descriptors representing all inputs.
    for (auto& input : inputs_) {
      input->connect();
      if (input->fd_ < 0) continue;  // No file descriptor at the moment.
      max_fd = std::max(max_fd, input->fd_);
      ++num_fds;
      FD_SET(input->fd_, &fds);
    }

    // Wait for up to 1 second for any of them to have data.
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int rv = select(max_fd + 1, &fds, NULL, NULL, &tv);
    //fprintf(stderr, "Waited for %d fds, %d are ready\n", num_fds, rv);

    // If there are any inputs with data, loop through them.
    if (rv > 0) {
      for (auto& input : inputs_) {
        if (input->fd_ < 0) continue;  // No file descriptor at the moment.
        if (FD_ISSET(input->fd_, &fds)) {
          // Read all the data available in each input.
          while (input->read()) {}
        }
      }
    }
  }
}

// Input from a PS3 controller.
class PS3 : public Input {
 public:
  PS3(Output::Player player, const char* path);
  virtual ~PS3() {}

 protected:
  virtual bool connect() override final;
  virtual bool read() override final;

 private:
  // The PS3 buttons.  The enum value is the corresponding event number.
  typedef enum {
    SELECT = 0,
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

  // The PS3 analog sticks.  The enum value is the corresponding stick number.
  typedef enum {
    LEFT_ANALOG = 0,
    RIGHT_ANALOG,
  } Stick;

  // Which player this input is tied to.
  const Output::Player player_;

  // The path to the input device.
  const char* path_;

  // A map from event numbers to output buttons.
  const std::map<int, Output::Button> button_map_;

  // A pair of output buttons, representing the outputs to use for the negative
  // and positive ends of one analog axis.
  typedef std::pair<Output::Button, Output::Button> AxisOutputs;
  // A pair of AxisOutputs, representing the vertical and horizontal axes of
  // one analog stick.
  typedef std::pair<AxisOutputs, AxisOutputs> StickOutputs;
  // A map from analog stick numbers to output buttons.
  const std::map<int, StickOutputs> stick_map_;
};

PS3::PS3(Output::Player player, const char* path)
    : Input(-1),
      player_(player),
      path_(path),
      // Not every PS3 button has a mapping here.
      button_map_({
        { UP, Output::UP },
        { RIGHT, Output::RIGHT },
        { DOWN, Output::DOWN },
        { LEFT, Output::LEFT },
        { SQUARE, Output::A },
        { X, Output::B },
        { CIRCLE, Output::C },
        { R2, Output::C },  // Alternate C
        { START, Output::START },
        { TRIANGLE, Output::START },  // Alternate start
      }),
      stick_map_({
        { LEFT_ANALOG, {  // Alternate D-pad
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
      // Disconnect.
      close(fd_);
      fd_ = -1;
    }
    return false;
  }

  Output* output = Output::instance();

  // Handle button events.
  if (event.type & JS_EVENT_BUTTON) {
#ifdef DEBUG
    fprintf(stderr, "number: 0x%02x, value: 0x%04x\n",
            event.number, event.value);
#endif
    // If this button is mapped, write to the output.
    auto it = button_map_.find(event.number);
    if (it != button_map_.end()) {
      output->write(player_, it->second, event.value);
    }
  }

  // Handle axis events.
  if (event.type & JS_EVENT_AXIS) {
#ifdef DEBUG
    fprintf(stderr, "number: 0x%02x, value: %d\n",
            event.number, event.value);
#endif
    // If this stick is mapped, interpret the analog value.
    int stick_number = event.number >> 1;
    int axis_number = event.number & 1;
    auto it = stick_map_.find(stick_number);
    if (it != stick_map_.end()) {
      // Decide which axis it is.
      auto stick_outputs = it->second;
      auto axis_outputs =
          axis_number ? stick_outputs.first : stick_outputs.second;

      if (event.value < -20000) {
        // For large negative values, output true to the negative output,
        // false to the positive output.
        output->write(player_, axis_outputs.first, true);
        output->write(player_, axis_outputs.second, false);
      } else if (event.value > 20000) {
        // For large positive values, output false to the negative output,
        // true to the positive output.
        output->write(player_, axis_outputs.first, false);
        output->write(player_, axis_outputs.second, true);
      } else {
        // For values that are in the middle, output false to both outputs.
        output->write(player_, axis_outputs.first, false);
        output->write(player_, axis_outputs.second, false);
      }
    }
  }

  return true;
}

// Input from the keyboard (stdin).
// Used for testing without bluetooth.
class Keyboard : public Input {
 public:
  Keyboard();
  virtual ~Keyboard() {}

 protected:
  virtual bool connect() override final;
  virtual bool read() override final;

 private:
  // Restore normal terminal echo settings.
  static void restore_echo();

  // Disable echo of input on the terminal.
  static void disable_echo();

  // The original terminal flags, saved by disable_echo(),
  // restored by restore_echo().
  static int orig_lflag_;

  // A map of characters to output buttons.
  const std::map<int, Output::Button> button_map_;
};

// static
int Keyboard::orig_lflag_;

Keyboard::Keyboard()
    : Input(STDIN_FILENO),  // Always read from stdin
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
  // Make stdin non-blocking.
  make_fd_nonblocking();
  // Disable echo of input on the terminal.
  disable_echo();
  // At exit, restore terminal settings.
  atexit(restore_echo);
}

bool Keyboard::connect() {
  // Always connected.
  return true;
}

bool Keyboard::read() {
  // Read one character.
  char c;
  int bytes = ::read(STDIN_FILENO, &c, 1);
  if (bytes != 1) {
    return false;
  }

  // Convert the character to lowercase, for use in the button map.
  char lower_c = c | 0x20;
  // If the original character was lowercase, we output to P1, otherwise P2.
  bool is_lower = lower_c == c;
  Output::Player player = is_lower ? Output::P1 : Output::P2;
  Output* output = Output::instance();

  // If the character is in the map,
  // press the corresponding output button briefly.
  auto it = button_map_.find(lower_c);
  if (it != button_map_.end()) {
#ifdef DEBUG
    fprintf(stderr, "'%c'\n", c);
#endif
    output->write(player, it->second, 1);
    usleep(100 * 1000);  // 0.1 seconds
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
  // Initialize output.
  Output::init();
  // Listen on stdin, for debugging.
  Keyboard keyboard;
  // Listen for PS3 controllers for both P1 and P2.
  PS3 ps3_p1(Output::P1, "/dev/input/js0");
  PS3 ps3_p2(Output::P2, "/dev/input/js1");
  // Read input forever.  Never returns.
  Input::poll();
  return 0;
}
