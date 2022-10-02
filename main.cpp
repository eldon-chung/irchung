#include <atomic>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <string>
#include <thread>

#include "connection.h"
#include "irc_client.h"

using namespace std::chrono_literals;
int main() {
  std::string input_buf;
  zpr::println("Enter a nickname:");
  std::getline(std::cin, input_buf);
  std::string nickname{input_buf};

  zpr::println("Enter a username:");
  std::getline(std::cin, input_buf);
  std::string username{input_buf};

  IRCClient irc_client{"10.200.0.1", 14429, nickname, username};
  std::this_thread::sleep_for(5s);

  // default channel to join
  // irc_client.join_channel("aoeu2");
  // irc_client.switch_channel("aoeu2");

  while (true) {
    std::getline(std::cin, input_buf);
    if (input_buf == "/q") {
      break;
    }

    if (input_buf == "/list") {
      irc_client.list_channels();
      continue;
    }

    if (input_buf.starts_with("/switch ")) {
      std::string channel_name{input_buf.substr(8)};
      irc_client.switch_channel(channel_name);
      continue;
    }

    if (input_buf.starts_with("/join ")) {
      std::string channel_name{input_buf.substr(6)};
      // switch channels
      irc_client.join_channel(channel_name);
      continue;
    }

    // default case is to ignore anything unrecognised that starts with a "/"
    if (input_buf.starts_with("/")) {
      continue;
    }

    irc_client.send_msg(input_buf);
  }
  irc_client.quit();
}