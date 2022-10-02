#include <cassert>
#include <iostream>
#include <string>
#include <vector>

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
      // if it's a colon we just yeet the entire thing in wholesale
      token_list.push_back(curr_str_view);
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

int main() {
  std::string test{
      ":zhiayang!zhiayang@gluon.lan PRIVMSG #aoeu2 :one two three four"};
  auto token_list = tokenise(test);

  // for (auto token : token_list) {
  //   std::cout << token << std::endl;
  // }

  if (token_list.at(1) == "PRIVMSG") {
    std::cout << token_list.back();
  }
}