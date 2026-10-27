#include "hex.hpp"

#include <string>
#include <array>
#include <iostream>
#include <optional>

int hex_to_val(char c) {
    if (c >= '0' && c <= '9') return(c - '0');
    if (c >= 'A' && c <= 'F') return(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

int hex_pair_to_byte(char high, char low){
    int a = hex_to_val(high);
    int b = hex_to_val(low);

    if (a == -1 || b == -1){
        return(-1);
    }

    return((a << 4) | b);
}


std::optional<std::array<uint8_t, 14>> parse_hex(const std::string& hex){
    if (hex.size() != 28) return std::nullopt;

    std::array<uint8_t, 14> bytes;
    for (int i =0; i < 14; i++){
        int temp = hex_pair_to_byte(hex[2*i], hex[2*i + 1]);
        if (temp != -1){
            bytes[i] = static_cast<uint8_t>(temp);
        }
        else{
            return std::nullopt;
        }

    }
    return bytes;
}