#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <optional>


using Msg = std::array<uint8_t, 14>;

struct Velocity {
    double speed_kt;
    double angle_deg;       // track if ground speed, heading if airspeed
    bool is_airspeed;     // false = ground speed (ST 1/2), true = airspeed (ST 3/4)
    bool is_tas;          // only meaningful if is_airspeed
    bool angle_valid;
    std::optional<int> vrate_fpm;
};

bool crc_ok(const Msg& msg);

int get_df(const Msg& msg);
int get_ca(const Msg& msg);
int get_tc(const Msg& msg);
uint32_t get_icao(const Msg& msg);


std::optional<std::string> get_callsign(const Msg& msg);  // TC 1-4
std::optional<int> get_altitude(const Msg& msg);  // TC 9-18, feet
std::optional<Velocity> get_velocity(const Msg& msg);  // TC 19