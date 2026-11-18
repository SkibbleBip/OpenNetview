

#include "Command.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>

template <typename... Args>
int WebConnect::sendCommand(COMMAND_TYPE command_type, Args... args) {

  std::string command_str;

  switch (command_type) {
  case COMMAND_TYPE::NOOP: {
    // do nothing
    return 0;
  };
  case COMMAND_TYPE::MOVE: {
    command_str = "move";
    break;
  };
  case COMMAND_TYPE::HOME: {
    command_str = "goto";
    break;
  }

  default: {
    std::cerr << "Unexpected command: " << (int)command_type << "... Ignoring."
              << std::endl;
    return -1;
  };
  }

  std::stringstream command_msg;

  command_msg << command_str << " ";

  ((command_msg << args << " "), ...);
  // black magic

  if (command_msg.str().length() > MAX_PAYLOAD_SIZE) {
    std::cerr << "Error, command is too long... ignoring" << std::endl;
    return -1;
  }

  Command cmd = {0};

  cmd.startByte = START_COMMAND;
  memcpy(cmd.payload_bytes, command_msg.str().c_str(),
         command_msg.str().length());
  uint16_t crc = crc16(cmd.payload_bytes, sizeof(cmd.payload_bytes));
  cmd.crc = htons(crc);

  std::cout << "Sending command: " << command_msg.str() << std::endl;

  int ret = 0;
  ssize_t total = 0;
  do {
    ret = write(command_socket, &cmd, sizeof(cmd) - total);

    if (ret < 0) {
      m_error = errno;
      std::cerr << "Failed to write to command socket: " << m_error
                << std::endl;
      return m_error;
    }
    total += ret;
  } while (total < sizeof(cmd));

  return 0;
}
