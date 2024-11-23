#pragma once
#include "esphome/core/component.h"
#include "esphome/components/time/real_time_clock.h"
#include <map>

namespace esphome {
namespace transit_511 {

// TODO may need to add a way to remove a route that is no longer active (ex: NOWL)

struct source {
    std::string url;
    //uint32_t refresh_ms;
};

// TODO add color, rail, and more style metadata to this
struct transitRouteETA {
    // line, direction & stop reference number
    std::string reference;
    // line Name
    std::string Name;
    // IB or OB
    std::string Direction;
    // if tracked live, timestamp of data sample, otherwise 0
    time_t RecordedAtTime;
    // timestamp of expected arrival
    time_t ETA;
    // timestamp when the API was queried for this result
    time_t ResponseTimestamp;
    // live tracking
    bool live;
};


class Transit511 : public Component {
    public:
        void dump_config() override;
        void setup() override;
        void loop() override;
        float get_setup_priority() const override { return setup_priority::PROCESSOR; };

        void add_source(std::string url);
        void set_time(time::RealTimeClock *rtc) { rtc_ = rtc; }
        void set_refresh(uint32_t refresh_ms) { this->refresh_ms_ = refresh_ms; };

    //protected:
        // logic
        // TODO make protected
        void parse_transit_response(std::string body);

        // settings
        std::vector<source> sources_;
        uint32_t refresh_ms_;
        time::RealTimeClock *rtc_;

        // transit data
        // map of all routes per stop to sorted ETAs
        std::map<std::string, std::vector<transitRouteETA>> reference_routes;
        // map of all route lines to sorted ETAs
        std::map<std::string, std::vector<const transitRouteETA*>> routes;
        // all ETAs merged and sorted
        std::vector<const transitRouteETA*> allETAs;

        void sortETA();
        void addETAs(std::vector<transitRouteETA> etas);
        void debugPrint();

        // scheduling
        int64_t get_time_ns_();
        void set_next_call_ns_();
        int64_t next_call_ns_{0};
        int64_t last_time_ms_{0};
        uint32_t millis_overflow_counter_{0};
};

void debug_print_tm(tm t);
time_t timeFromJSON(const char *str);

bool etaCmp(const transitRouteETA* a, const transitRouteETA* b);
bool etaCmpRef(const transitRouteETA& a, const transitRouteETA& b);


} // namespace transit_511
} // namespace esphome

// allow accessing timezone offset directly
extern long _timezone;
