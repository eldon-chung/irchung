#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "connection.h"
#include "names.h"
#include "session.h"
#include "zpr.h"

inline constexpr int CODE_MOTD = 372;
inline constexpr int CODE_WELCOME = 1;

class IRCClient {
  std::atomic<bool> m_has_joined;
  Connection m_connection;
  std::string m_read_buffer;
  std::list<Session> m_session_list;
  std::thread m_thread;
  Session *m_current_channel;
  std::string m_uname;
  std::string m_nickname;

public:
  ~IRCClient() {
    m_connection.close();
    m_thread.join();
  }

  IRCClient(std::string hostname, int port_num, std::string uname, std::string nickname)
      : m_has_joined(false), m_connection{hostname, port_num}, m_current_channel(nullptr),
        m_uname(uname), m_nickname(nickname) {
    m_thread = std::thread(&IRCClient::reader_thread, this);
    register_client(m_nickname, m_uname);
  }

private:
  void pong(std::string const &target_name) {
    m_connection.write(zpr::sprint("PONG :{}", target_name));
  }

  auto find_session_from_name(std::list<Session> &session_list, std::string const &channel_name) {
    return std::find_if(session_list.begin(), session_list.end(), [&channel_name](Session &sesh) {
      return sesh.channel_name() == channel_name;
    });
  }

  void reader_thread() {
    std::string read_buf;
    while (m_connection.read(read_buf, 1024) >= 0) {
      m_read_buffer.append(read_buf);

      // continually
      auto maybe_line = try_get_line(m_read_buffer);
      while (maybe_line.has_value()) {
        // zpr::fprintln(stderr, "reader_thread obtained line:{}", *maybe_line);
        handle_server_str(*maybe_line);
        maybe_line = try_get_line(m_read_buffer);
      }
    }
  }

  void handle_server_str(std::string const &str) {
    std::vector<std::string_view> token_list = tokenise(str);

    // if (!token_list.empty()) {
    // zpr::fprintln(stderr, "received token list: {}", token_list);
    // }

    std::string_view command = token_list.at(0);
    std::optional<int> maybe_numeric_code = try_get_numeric_code(token_list);

    if (maybe_numeric_code.has_value()) {
      if (maybe_numeric_code.value() == CODE_MOTD || maybe_numeric_code.value() == CODE_WELCOME) {
        zpr::println("{}", token_list.back());
      }
      // std::cerr << "numeric code: " << maybe_numeric_code.value() << std::endl;
      return;
    }

    if (command == "PING") {
      pong(std::string{token_list.at(1)});
      return;
    }
    std::optional<std::string> origin_nick = std::nullopt;
    if (command.starts_with(":")) {
      origin_nick = remove_hostname(command.substr(1));
    }

    // :nodle!nodle@thonkpad.lan JOIN :#aoeu2
    if (token_list.at(1) == "JOIN") {
      std::string joined_user_name{token_list.at(0)};
      std::string channel_name{token_list.at(2)};
      // std::string_view user_name_view{joined_user_name};
      size_t excl_idx = joined_user_name.find_first_of("!");

      if (excl_idx != std::string::npos) {
        joined_user_name = joined_user_name.substr(0, excl_idx);
      }
      zpr::fprintln(stderr, "joined user name: ({})", joined_user_name);
      joined_user_name.erase(0, 1);
      if (joined_user_name == m_nickname || joined_user_name == m_uname) {
        m_has_joined = true;
        m_has_joined.notify_all();
      } else {
        auto session = find_session_from_name(m_session_list, channel_name);
        if (session == m_session_list.end()) {
          zpr::println("could not find channel ({}) name of person ({}) who joined", channel_name,
                       m_nickname);
          exit(1);
        }
        std::string joined_msg = zpr::sprint("{}: {} has joined!", channel_name, m_nickname);
        session->into_buffer(std::move(joined_msg));
      }
    }

    // I assume that PRIVMSG is always as the second token
    if (token_list.at(1) == "PRIVMSG") {
      assert(origin_nick.has_value());
      std::string channel_name{token_list.at(2)};
      auto session = find_session_from_name(m_session_list, channel_name);
      if (session == m_session_list.end()) {
        zpr::println("could not find channel ({}) name of person ({}) who joined", channel_name,
                     m_nickname);
        exit(1);
      }
      if (session->channel_name() == m_current_channel->channel_name()) {
        zpr::println("{} says: {}", origin_nick.value(), token_list.back());
      } else {
        session->into_buffer(zpr::sprint("{} says: {}", origin_nick.value(), token_list.back()));
      }
      return;
    }
  }

  void register_client(std::string const &nickname, std::string const &username) {
    std::string nick_reg_string = zpr::sprint("NICK {}\r\n", nickname);
    std::string user_reg_string = zpr::sprint("USER {} * * :{}\r\n", username, username);

    m_connection.write(nick_reg_string);
    m_connection.write(user_reg_string);
  }

public:
  void join_channel(std::string const &channel_name) {
    // operation should be idempotent if the channel already exists
    auto session = find_session_from_name(m_session_list, channel_name);
    if (session != m_session_list.end()) {
      zpr::println("Client: Channel [#{}] already exists!");
      return;
    }

    zpr::println("Client: Joining [#{}]...", channel_name);

    m_has_joined = false;
    std::string channel_join_string = zpr::sprint("JOIN #{}\r\n", channel_name);
    m_connection.write(channel_join_string);

    Session &constructed = m_session_list.emplace_back(zpr::sprint("#{}", channel_name));
    m_current_channel = &constructed;
    // zpr::fprint(stderr, "waiting for m_has_joined\n");
    m_has_joined.wait(false);
    zpr::println("Client: Joined!", channel_name);

    // zpr::fprint(stderr, "done\n");
  }

  void switch_channel(std::string const &channel_name) {
    assert(!m_session_list.empty());
    auto found_session = find_session_from_name(m_session_list, zpr::sprint("#{}", channel_name));
    if (found_session == m_session_list.end()) {
      zpr::println("Could not find channel [#{}]", channel_name);
      return;
    }
    zpr::println("Changing to channel [{}]", found_session->channel_name());

    // clear the message buffer
    found_session->print_and_clear_buffer();
    m_current_channel = &*found_session;
  }

  void send_msg(std::string const &msg) {
    std::string send_msg_string =
        zpr::sprint("PRIVMSG {} :{}\r\n", m_current_channel->channel_name(), msg);
    zpr::println("you said: {}", msg);
    m_connection.write(send_msg_string);
  }

  void quit() {
    m_connection.write("QUIT\r\n");
  }

  void list_channels() {
    if (m_session_list.empty()) {
      zpr::println("No channels joined!");
      return;
    }
    size_t idx = 0;
    zpr::println("CHANNEL LIST: ========================");
    for (Session &sesh : m_session_list) {
      zpr::println("{}:{}", idx, sesh.channel_name());
      idx++;
    }
    zpr::println("=============================");
  }
};