/*
库代码来自：https://github.com/spicetools/spicetools/tree/master/api/resources/arduino
删除了未使用的函数。
*/
#ifndef SPICEAPI_CONNECTION_H
#define SPICEAPI_CONNECTION_H

#include <stdint.h>

#ifndef SPICEAPI_INTERFACE
#define SPICEAPI_INTERFACE Serial
#endif

namespace spiceapi {

class Connection {
private:
  uint8_t* receive_buffer;
  size_t receive_buffer_size;

public:
  Connection(size_t receive_buffer_size);
  void reset();
  const char* request(char* json);
};
}

spiceapi::Connection::Connection(size_t receive_buffer_size) {
  this->receive_buffer = new uint8_t[receive_buffer_size];
  this->receive_buffer_size = receive_buffer_size;
  this->reset();
}

void spiceapi::Connection::reset() {

  // drop all input
  while (SPICEAPI_INTERFACE.available()) {
    SPICEAPI_INTERFACE.read();
  }
}

const char* spiceapi::Connection::request(char* json_data) {
  auto json_len = strlen(json_data) + 1;

  // send
  auto send_result = SPICEAPI_INTERFACE.write((const char*)json_data, (int)json_len);
  SPICEAPI_INTERFACE.flush();
  if (send_result < (int)json_len) {
    return "";
  }

  // receive
  size_t receive_data_len = 0;
  while (SPICEAPI_INTERFACE) {

    // read single byte
    auto b = SPICEAPI_INTERFACE.read();
    if (b < 0) continue;
    receive_buffer[receive_data_len++] = b;

    // check for buffer overflow
    if (receive_data_len >= receive_buffer_size) {
      this->reset();
      return "";
    }

    // check for message end
    if (receive_buffer[receive_data_len - 1] == 0)
      break;
  }

  // return resulting json
  return (const char*)&receive_buffer[0];
}

#endif  //SPICEAPI_CONNECTION_H
