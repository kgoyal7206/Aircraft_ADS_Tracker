#include "decode.hpp"
#include <cmath>

static constexpr double PI = 3.14159265358979323846;

static uint32_t bits(const Msg& m, int start, int len) {
    uint32_t v = 0;
    for (int i = start - 1; i < start - 1 + len; ++i)
        v = (v << 1) | ((m[i / 8] >> (7 - i % 8)) & 1);
    return v;
}
// ME field starts at message bit 33
static uint32_t me(const Msg& m, int start, int len){
    return bits(m, 32 + start, len); 
}

bool crc_ok(const Msg& m) {
    uint32_t crc = 0;
    for (int i = 0; i < 11; ++i) {
        crc ^= uint32_t(m[i]) << 16;
        for (int b = 0; b < 8; ++b) {
            crc <<= 1;
            if (crc & 0x1000000) crc ^= 0x1FFF409; 
    }
    }
    uint32_t parity = (m[11] << 16) | (m[12] << 8) | m[13];
    return (crc & 0xFFFFFF) == parity; 
    
}
int get_df(const Msg& m){ 
    return m[0] >> 3; 
}
int get_ca(const Msg& m){ 
    return m[0] & 0x07;
}
int get_tc(const Msg& m){ 
    return m[4] >> 3;
}
uint32_t get_icao(const Msg& m){ 
    return (m[1] << 16) | (m[2] << 8) | m[3];
}

static bool is_adsb(const Msg& m) {
    int df = get_df(m);
    return df == 17 || df == 18;
}

std::optional<std::string> get_callsign(const Msg& m) {
    if (!is_adsb(m)) return std::nullopt;
    int tc = get_tc(m);
    if (tc < 1 || tc > 4) return std::nullopt;

    static const char* charset =
        "#ABCDEFGHIJKLMNOPQRSTUVWXYZ#####"
        " ###############0123456789######";

    std::string cs;
    for (int i = 0; i < 8; ++i)
        cs += charset[me(m, 9 + i * 6, 6)];

    while (!cs.empty() && (cs.back() == ' ' || cs.back() == '#')) cs.pop_back();
    return cs;
}

std::optional<int> get_altitude(const Msg& m) {
    if (!is_adsb(m)) return std::nullopt;
    int tc = get_tc(m);
    if (tc < 9 || tc > 18) return std::nullopt;   // TC 20-22 = GNSS alt, different encoding

    uint32_t alt = me(m, 9, 12);
    bool q = (alt >> 4) & 1;
    if (!q) return std::nullopt;  // Gillham (100 ft) code, rare, not handled

    uint32_t n = ((alt >> 5) << 4) | (alt & 0x0F); // remove Q bit
    return int(n) * 25 - 1000;
}


std::optional<Velocity> get_velocity(const Msg& m) {
    if (!is_adsb(m)) return std::nullopt;
    if (get_tc(m) != 19) return std::nullopt;

    int st = me(m, 6, 3);
    if (st < 1 || st > 4) return std::nullopt;

    Velocity v{};
    int mult = (st == 2 || st == 4) ? 4 : 1;

    if (st == 1 || st == 2) {
        int dew = me(m, 14, 1), vew = me(m, 15, 10);
        int dns = me(m, 25, 1), vns = me(m, 26, 10);
        if (vew == 0 || vns == 0) return std::nullopt; // not available

        double vx = (vew - 1) * mult * (dew ? -1 : 1);
        double vy = (vns - 1) * mult * (dns ? -1 : 1);

        v.speed_kt  = std::hypot(vx, vy);
        v.angle_deg = std::fmod(std::atan2(vx, vy) * 180.0 /PI  + 360.0, 360.0);
        v.angle_valid = true;
        v.is_airspeed = false;
    } else {
        int hdg_ok = me(m, 14, 1);
        int hdg    = me(m, 15, 10);
        int as_typ = me(m, 25, 1);
        int as     = me(m, 26, 10);
        if (as == 0) return std::nullopt;

        v.speed_kt    = (as - 1) * mult;
        v.angle_valid = hdg_ok;
        v.angle_deg   = hdg * 360.0 / 1024.0;
        v.is_airspeed = true;
        v.is_tas      = as_typ;
    }

    int vr_sign = me(m, 37, 1);
    int vr      = me(m, 38, 9);
    if (vr != 0){
        v.vrate_fpm = (vr - 1) * 64 * (vr_sign ? -1 : 1);
    }

    return v;
}