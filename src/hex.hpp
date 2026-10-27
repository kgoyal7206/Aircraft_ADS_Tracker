#pragma once
#include <string>
#include <array>
#include <cstdint>

#include "decode.hpp"

int hex_to_val(char c);


int hex_pair_to_byte(char high, char low);


std::optional<std::array<uint8_t, 14>> parse_hex(const std::string& hex);

