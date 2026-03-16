#ifndef TIMEUTILS_H
#define TIMEUTILS_H

#include <string>
#include <ctime>
#include <cstdint>
#include <chrono>
#include <iomanip>

const double TI_TICK_NS = 4.0;       // 1 TI tick = 4 ns
const double TI_TICK_SEC = 4e-9;     // 1 TI tick = 4e-9 s

struct TimeBase {
    bool   valid = false;
    time_t prestart_unix = 0;
    time_t go_unix = 0;
    uint64_t ti_base = 0;
};

inline std::string toUTC(time_t base_sec, int64_t ns_offset) {
    if (base_sec == 0) return "N/A";

    try {
        auto base_tp = std::chrono::system_clock::from_time_t(base_sec);
        auto total_tp = base_tp + std::chrono::nanoseconds(ns_offset);
        time_t total_sec = std::chrono::system_clock::to_time_t(total_tp);
        std::tm* utc_tm = std::gmtime(&total_sec);
        
        if (utc_tm) {
            char buf[64];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", utc_tm);
            int64_t ns = ns_offset % 1000000000;
            if (ns < 0) ns += 1000000000;
            char dec[16];
            std::snprintf(dec, 16, ".%06lld", (long long)(ns / 1000));
            return std::string(buf) + dec + " UTC";
        }
    } catch (...) {}
    return "Invalid Time";
}

inline std::string toEDT(time_t base_sec, int64_t ns_offset) {
    if (base_sec == 0) return "N/A";

    try {
        auto base_tp = std::chrono::system_clock::from_time_t(base_sec);
        auto total_tp = base_tp + std::chrono::nanoseconds(ns_offset);
        time_t total_sec = std::chrono::system_clock::to_time_t(total_tp - std::chrono::hours(4));
        std::tm* edt_tm = std::gmtime(&total_sec);
        
        if (edt_tm) {
            char buf[64];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", edt_tm);
            int64_t ns = ns_offset % 1000000000;
            if (ns < 0) ns += 1000000000;
            char dec[16];
            std::snprintf(dec, 16, ".%06lld", (long long)(ns / 1000));
            return std::string(buf) + dec + " EDT";
        }
    } catch (...) {}
    return "Invalid Time";
}

inline std::string calcAbsoluteTimeEDT(const TimeBase& tb, uint64_t ti_ts) {
    if (!tb.valid || ti_ts == 0 || tb.go_unix == 0) return "N/A";
    
    int64_t ti_diff = static_cast<int64_t>(ti_ts - tb.ti_base);
    int64_t ns_diff = ti_diff * 4; // 4ns per tick
    return toEDT(tb.go_unix, ns_diff);
}

inline std::string calcAbsoluteTimeUTC(const TimeBase& tb, uint64_t ti_ts) {
    if (!tb.valid || ti_ts == 0 || tb.go_unix == 0) return "N/A";
    
    int64_t ti_diff = static_cast<int64_t>(ti_ts - tb.ti_base);
    int64_t ns_diff = ti_diff * 4;
    return toUTC(tb.go_unix, ns_diff);
}

inline double calcRelativeTimeSec(const TimeBase& tb, uint64_t ti_ts) {
    if (!tb.valid || ti_ts == 0) return -1.0;
    int64_t ti_diff = static_cast<int64_t>(ti_ts - tb.ti_base);
    return ti_diff * TI_TICK_SEC;
}

#endif // TIMEUTILS_H