#pragma once

#include <algorithm>
#include <cassert>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "session.h"
#include "zpr.h"

std::string remove_hostname(std::string_view str);
std::vector<std::string_view> tokenise(std::string const &str);
std::optional<std::string> try_get_line(std::string &m_read_buffer);
std::optional<int>
try_get_numeric_code(std::vector<std::string_view> const &token_list);