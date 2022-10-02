#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "zpr.h"

class Session {
  std::string m_channel_name;
  std::vector<std::string> m_buffered_messages;
  std::mutex m_mut;

  // deal with the copy/move constructors and assignment operators some other
  // time

public:
  Session(std::string channel_name) : m_channel_name(channel_name) {
  }

  void print_and_clear_buffer() {
    std::scoped_lock lk(m_mut);
    for (std::string &msg : m_buffered_messages) {
      zpr::println("{}", msg);
    }
    m_buffered_messages.clear();
  }

  void into_buffer(std::string &&new_message) {
    std::scoped_lock lk(m_mut);
    m_buffered_messages.push_back(std::move(new_message));
  }

  std::string channel_name() {
    return m_channel_name;
  }
};
