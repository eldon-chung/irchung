#pragma once

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <netinet/ip.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "zpr.h"

class Connection {
  int m_file_descriptor;

public:
  Connection(std::string const &hostname, int port_num) {
    m_file_descriptor = socket(PF_INET, SOCK_STREAM, 0);
    if (m_file_descriptor == -1) {
      fprintf(stderr, "error opening socket: %s\n", strerror(errno));
      exit(1);
    }

    struct addrinfo hints = {
        .ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG),
        .ai_family = AF_INET,
    };

    int yes = 1;
    setsockopt(this->m_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes,
               sizeof(yes));

    struct addrinfo *info = nullptr;
    int res = getaddrinfo(hostname.c_str(), std::to_string(port_num).c_str(),
                          &hints, &info);

    if (res == -1) {
      fprintf(stderr, "error getting address info %s\n", strerror(errno));
      exit(1);
    }

    // starts tcp with this socket (on this fd)
    res = connect(m_file_descriptor, info->ai_addr, info->ai_addrlen);
    freeaddrinfo(info);
    if (res == -1) {
      fprintf(stderr, "error getting address info %s\n", strerror(errno));
      exit(1);
    }
  }

  // important shit
  ~Connection() {
    this->close();
  }

  Connection(Connection const &) = delete;
  Connection &operator=(Connection const &) = delete;
  Connection(Connection &&other) {
    m_file_descriptor = other.m_file_descriptor;
    other.m_file_descriptor = -1;
  }

  Connection &operator=(Connection &&other) {
    Connection temp{std::move(other)};
    std::swap(this->m_file_descriptor, temp.m_file_descriptor);
    return *this;
  }

  ssize_t read(std::string &str, size_t count) {
    str.resize(count);
    ssize_t num_read = ::read(m_file_descriptor, (void *)str.data(), count);
    if (num_read >= 0) {
      str.resize(num_read);
    }
    // zpr::fprintln(stderr, "Connection received bytes:{}", str);
    return num_read;
  }

  ssize_t write(std::string const &str) {
    // std::cerr << "Connection: writing str: " << str << std::endl;
    ssize_t num_write = ::write(m_file_descriptor, str.data(), str.size());
    return num_write;
  }

  void close() {
    if (m_file_descriptor != -1) {
      ::shutdown(m_file_descriptor, SHUT_RDWR);
      ::close(m_file_descriptor);
      m_file_descriptor = -1;
    }
  }
};