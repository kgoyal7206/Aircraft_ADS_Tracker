// tests.cpp
#include "decode.hpp"
#include "hex.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

static int g_pass = 0, g_fail = 0;

#define CHECK(cond) do { \
    if (cond) ++g_pass; \
    else { ++g_fail; std::cerr << __FILE__ << ":" << __LINE__ << "  FAIL: " #cond "\n"; } \
} while (0)

#define CHECK_NEAR(a, b, tol) CHECK(std::fabs(double(a) - double(b)) <= (tol))

static Msg msg(const char* hex) {
    auto r = parse_hex(hex);
    if (!r) { std::cerr << "Bad test vector: " << hex << "\n"; std::abort(); }
    return *r;
}

// Known messages from "The 1090 MHz Riddle"
static const char* IDENT = "8D4840D6202CC371C32CE0576098";   // KLM1023
static const char* VEL   = "8D485020994409940838175B284F";   // ground speed, sub-type 1
static const char* POS_E = "8D40621D58C382D690C8AC2863A7";   // airborne position, even
static const char* POS_O = "8D40621D58C386435CC412692AD6";   // airborne position, odd

void test_hex() {
    CHECK(parse_hex(IDENT).has_value());
    CHECK(parse_hex("8d4840d6202cc371c32ce0576098") == parse_hex(IDENT)); // lowercase
    CHECK(!parse_hex("8D4840D6202CC371C32CE057609").has_value());          // 27 chars
    CHECK(!parse_hex("8D4840D6202CC371C32CE05760988").has_value());        // 29 chars
    CHECK(!parse_hex("8D4840D6202CC371C32CE05760G8").has_value());         // invalid char
    CHECK(!parse_hex("").has_value());
}

void test_crc() {
    CHECK(crc_ok(msg(IDENT)));
    CHECK(crc_ok(msg(VEL)));
    CHECK(crc_ok(msg(POS_E)));
    CHECK(crc_ok(msg(POS_O)));

    // Flipping any single bit must break the CRC
    Msg m = msg(IDENT);
    for (int bit = 0; bit < 112; ++bit) {
        Msg f = m;
        f[bit / 8] ^= uint8_t(1u << (7 - bit % 8));
        if (crc_ok(f)) { CHECK(false); std::cerr << "  bit " << bit << " flip passed CRC\n"; }
    }
    CHECK(true);
}

void test_header_fields() {
    Msg m = msg(IDENT);
    CHECK(get_df(m) == 17);
    CHECK(get_ca(m) == 5);
    CHECK(get_icao(m) == 0x4840D6);
    CHECK(get_tc(m) == 4);
    CHECK(get_tc(msg(VEL)) == 19);
    CHECK(get_tc(msg(POS_E)) == 11);
}

void test_callsign() {
    auto cs = get_callsign(msg(IDENT));
    CHECK(cs.has_value() && *cs == "KLM1023");
    CHECK(!get_callsign(msg(VEL)).has_value());    // wrong type code
    CHECK(!get_callsign(msg(POS_E)).has_value());
}

void test_altitude() {
    auto a = get_altitude(msg(POS_E));
    CHECK(a.has_value() && *a == 38000);
    auto b = get_altitude(msg(POS_O));
    CHECK(b.has_value() && *b == 38000);
    CHECK(!get_altitude(msg(IDENT)).has_value());  // wrong type code
}

void test_velocity() {
    auto v = get_velocity(msg(VEL));
    CHECK(v.has_value());
    if (!v) return;
    CHECK_NEAR(v->speed_kt, 159.20, 0.01);
    CHECK_NEAR(v->angle_deg, 182.88, 0.01);
    CHECK(v->angle_valid);
    CHECK(!v->is_airspeed);
    CHECK(v->vrate_fpm.has_value() && *v->vrate_fpm == -832);  // assumes optional<int>
    CHECK(!get_velocity(msg(IDENT)).has_value());
}

void test_wrong_df_rejected() {
    // Same bytes but DF changed to 11 (0x8D -> 0x5D): decoders must refuse
    Msg m = msg(IDENT);
    m[0] = 0x5D;
    CHECK(get_df(m) == 11);
    CHECK(!get_callsign(m).has_value());
}

int main() {
    test_hex();
    test_crc();
    test_header_fields();
    test_callsign();
    test_altitude();
    test_velocity();
    test_wrong_df_rejected();

    std::cout << g_pass << " passed, " << g_fail << " failed\n";
    return g_fail ? 1 : 0;
}