// SPDX-FileCopyrightText: (c) 2023 University of Illinois Urbana-Champaign
// SPDX-FileCopyrightText: (c) 2023 Jiawei Liu <jiawei6@illinois.edu>
//
// SPDX-License-Identifier: Apache-2.0

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <string_view>
#include <utility>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>


namespace llamuta {
using byte_view = std::basic_string_view<uint8_t>;
std::vector<std::vector<uint8_t>> dictionary_data{};
std::vector<byte_view> dictionary{};
}

struct Logger {
  std::ofstream file;
  Logger(std::string_view path) {
    file.open(path.data(), std::ios::out | std::ios::app | std::ios::binary);
    if (!file.is_open()) {
      std::cerr << "Failed to open " << path << std::endl;
    }
  }

  ~Logger() {
    file.close();
  }

  template <typename T>
  Logger &operator<<(const T &msg) {
    file << msg;
    return *this;
  }

  // behave like python's repr
  Logger &operator<<(const llamuta::byte_view &data) {
    for (auto& byte : data) {
      if (std::isalnum(byte)) {
        file << static_cast<char>(byte);
      } else if (std::isprint(byte)) {
        if (byte == '\\') {
          file << "\\\\";
        } else {
          file << static_cast<char>(byte);
        }
      } else if (std::isspace(byte)) {
        switch (static_cast<char>(byte)) {
          case ' ':
            file << ' ';
            break;
          case '\f':
            file << "\\f";
            break;
          case '\n':
            file << "\\n";
            break;
          case '\r':
            file << "\\r";
            break;
          case '\t':
            file << "\\t";
            break;
          case '\v':
            file << "\\v";
            break;
          default:
            assert(false);
        }
      } else {
        file << "\\x" << std::hex << std::setw(2) << static_cast<int>(byte) << std::dec;
      }
    }
    return *this;
  }

  Logger &operator<<(std::ostream& (*manip)(std::ostream&)) {
    manip(file);
    return *this;
  }

};
namespace llamuta {

Logger log("debug.log");

byte_view get_random_literal(unsigned int seed) {
  // log << "[llamuta]: dictionary size: " << dictionary.size() << std::endl;
  return dictionary[static_cast<size_t>(seed) % dictionary.size()];
}

std::pair<size_t, size_t>
match_literals(byte_view bytes, const std::vector<byte_view> &dictionary) {
  for (auto &literal : dictionary) {
    auto pos = bytes.find(literal);
    if (pos != byte_view::npos) {
      return std::make_pair(pos, literal.size());
    }
  }
  return {byte_view::npos, 0};
}

size_t perform_literal_replacement(uint8_t *data, size_t size, size_t max_size,
                                   std::pair<size_t, size_t> match,
                                   byte_view target_literal) {
  auto [pos, old_lit_size] = match;
  // log << "[llamuta]: Replacing literal\n" 
  //     << "[llamuta]: ---before:\n"
  //     << "[llamuta]: " << byte_view{data + pos, old_lit_size} << std::endl
  //     << "[llamuta]: ---after:\n" 
  //     << "[llamuta]: " << target_literal << std::endl;
  assert(pos < size && pos + old_lit_size <= size);
  assert(!target_literal.empty());
  assert(size - old_lit_size + target_literal.size() <= max_size);
  

  // Replace old literal (pos, lit_size) with new literal (pos,
  // target_literal.size())
  // Move the rest of the data to the right if literal sizes mismatch
  if (old_lit_size != target_literal.size()) {
    std::copy(data + pos + old_lit_size, data + size,
              data + pos + target_literal.size());
  }

  std::copy(target_literal.begin(), target_literal.end(), data + pos);

  return size + target_literal.size() - old_lit_size;
}

size_t load_dictionary(const std::string_view path) {
  // get path from env
  // log << "[llamuta]: Reading dictionary from " << path << std::endl;
  std::ifstream file(path.data(), std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "[llamuta]: Failed to open dictionary file: " << path << std::endl;
    return 0;
  }
  std::string line;
  std::vector<uint8_t> buffer;
  auto parse_line = [&](std::string &line) {
    buffer.clear();
    const char* p = line.c_str();
    while (*p) {
      if (*p == '\\') {
        switch (*(p+1)) {
          case '\\':
            buffer.push_back('\\');
            p += 2;
            break;
          case 'f':
            buffer.push_back('\f');
            p += 2;
            break;
          case 'n':
            buffer.push_back('\n');
            p += 2;
            break;
          case 'r':
            buffer.push_back('\r');
            p += 2;
            break;
          case 't':
            buffer.push_back('\t');
            p += 2;
            break;
          case 'v':
            buffer.push_back('\v');
            p += 2;
            break;
          // hex
          case 'x':
            buffer.push_back(static_cast<char>(std::stoi(std::string{p+2, 2}, nullptr, 16)));
            p += 4;
            break;
        }
      } else {
        buffer.push_back(*p);
        p += 1;
      }
    }
    dictionary_data.push_back(buffer);
    auto& data = dictionary_data.back();
    dictionary.push_back(byte_view{data.data(), data.size()});
  };
  while (std::getline(file, line)) {
    parse_line(line);
  }
  // for (auto entry : dictionary) {
  //   log << "[llamuta]: " << entry << std::endl;
  // }
  return dictionary.size();
}



} // namespace llamuta


extern "C" size_t LLVMFuzzerMutate(uint8_t *data, size_t size, size_t max_size);

extern "C" size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size,
                                          size_t max_size, unsigned int seed) {
  using byte_view = llamuta::byte_view;


  const std::string_view path = std::getenv("LLAMUTA_DICT");
  if (llamuta::dictionary.empty()) {
    llamuta::load_dictionary(path);
  }

  // TODO(@ganler): find ways to initialize the dictionary!
  auto match = llamuta::match_literals(byte_view{data, size}, llamuta::dictionary);

  if (match.first != byte_view::npos) {
    // Perform literal replacement
    llamuta::byte_view target_literal = llamuta::get_random_literal(seed);
    if (size - match.second + target_literal.size() > max_size) {
      return LLVMFuzzerMutate(data, size, max_size);
    }
    return llamuta::perform_literal_replacement(data, size, max_size, match,
                                                target_literal);
  } else if (seed % 2 == 0) {
    // Insert over random positions
    auto target_literal = llamuta::get_random_literal(seed);
    size_t random_pos = seed % size;
    size_t random_size = seed % (size - random_pos); 
    if (size - random_size + target_literal.size() > max_size) {
      return LLVMFuzzerMutate(data, size, max_size);
    }
    return llamuta::perform_literal_replacement(
        data, size, max_size, {random_pos, random_size}, target_literal);
  }

  // Perform default LLVM mutation
  return LLVMFuzzerMutate(data, size, max_size);
}
