#include "names.h"

std::string remove_hostname(std::string_view str) {
  size_t cutoff = str.find_first_of("@!");

  if (cutoff == std::string::npos) {
    return std::string{str};
  }
  return std::string{str.substr(0, cutoff)};
}

std::vector<std::string_view> tokenise(std::string const &str) {
  std::vector<std::string_view> token_list;
  std::string_view curr_str_view{str};

  if (curr_str_view.at(0) == ':') {
    // if starts with a colon we need to take the first thing out as a token
    //  and also keep the colon in
    size_t start_idx = curr_str_view.find_first_of(" ");
    token_list.push_back(std::string_view{curr_str_view.data(), start_idx});
    curr_str_view.remove_prefix(start_idx);

    // prayge that are still more tokens in this string;
    assert(!curr_str_view.empty() &&
           curr_str_view.find_first_not_of(" ") != std::string::npos);
  }

  // remove leading spaces
  size_t start_idx = curr_str_view.find_first_not_of(" ");
  curr_str_view.remove_prefix(start_idx);

  while (!curr_str_view.empty()) {
    assert(curr_str_view.at(0) != ' ');

    if (curr_str_view.at(0) == ':') {
      // if it's a colon we push the entire thing in without the first colon
      token_list.push_back(curr_str_view.substr(1));
      break;
    }

    size_t ending_idx = curr_str_view.find_first_of(" ");
    // if no more spaces return the entire thing
    if (ending_idx == std::string::npos) {
      token_list.push_back(std::string_view{curr_str_view});
      break;
    } else {
      token_list.push_back(std::string_view{curr_str_view.data(), ending_idx});
      curr_str_view.remove_prefix(ending_idx);
    }

    // now remove leading spaces again, quit if it's trailing spaces
    start_idx = curr_str_view.find_first_not_of(" ");
    if (start_idx == std::string::npos) {
      break;
    }
    curr_str_view.remove_prefix(start_idx);
  }
  return token_list;
}

std::optional<std::string> try_get_line(std::string &read_buffer) {
  zpr::fprintln(stderr, "try_get_line current buffer contents:{}", read_buffer);
  size_t found_idx = read_buffer.find("\r\n");
  if (found_idx == std::string::npos) {
    return std::nullopt;
  }

  std::string obtained_line{read_buffer.substr(0, found_idx)};
  read_buffer.erase(0, found_idx + 2);
  return obtained_line;
}

std::optional<int>
try_get_numeric_code(std::vector<std::string_view> const &token_list) {
  assert(!token_list.empty());

  auto is_triple_digit_at_idx = [&token_list](size_t idx) {
    return isdigit(token_list.at(idx).at(0)) &&
           isdigit(token_list.at(idx).at(1)) &&
           isdigit(token_list.at(idx).at(2));
  };

  int numeric_code_idx = -1;
  if (is_triple_digit_at_idx(0)) {
    numeric_code_idx = 0;
  } else if (token_list.size() >= 2 && is_triple_digit_at_idx(1)) {
    numeric_code_idx = 1;
  }

  // if (numeric_code_idx != -1) {
  //   std::cerr << "try_get_numeric_code: calling stoi on idx with contents:"
  //             << numeric_code_idx << " " <<
  //             token_list.at(numeric_code_idx);
  // }

  return (numeric_code_idx == -1)
             ? std::nullopt
             : std::optional{
                   std::stoi(std::string{token_list.at(numeric_code_idx)})};
}