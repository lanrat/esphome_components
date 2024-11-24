#pragma once
#include "esphome/core/component.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/http_request/http_request.h"
#include "esphome/components/wifi/wifi_component.h"
#include <map>

namespace esphome {
namespace transit_511 {

// TODO may need to add a way to remove a route that is no longer active (ex: NOWL)
// TODO refresh automatically on wifi connected
// TODO if line only has times in past, remove it

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
        void loop() override;
        float get_setup_priority() const override { return setup_priority::AFTER_WIFI; };

        void add_source(std::string url);
        void set_time(time::RealTimeClock *rtc) { rtc_ = rtc; }
        void set_wifi(wifi::WiFiComponent *wifi);
        void set_refresh(uint32_t refresh_ms) { this->refresh_ms_ = refresh_ms; };
        void set_http(http_request::HttpRequestComponent * http);
        void set_max_response_buffer_size(size_t max_response_buffer_size) {
            this->max_response_buffer_size_ = max_response_buffer_size;
        }

        void refresh(bool force=false);
        bool running() { return this->running_; };

        const std::map<std::string, std::vector<transitRouteETA>> get_reference_routes() { return this->reference_routes; };
        const std::map<std::string, std::vector<const transitRouteETA*>> get_routes() { return this->routes; };
        const std::vector<const transitRouteETA*> get_ETAs() { return this->allETAs; };

        void debug_print();
        bool has_data() {return this->routes.size() > 0;}; // TODO check that at least one ETA is in the future


    protected:
        void http_response_callback(std::shared_ptr<http_request::HttpContainer> response, std::string & body);
        void http_error_callback();
        http_request::HttpRequestSendAction<> *http_action_;
        size_t max_response_buffer_size_ = 0;

        // logic
        void parse_transit_response(std::string body);
        void sortETA();
        void addETAs(std::vector<transitRouteETA> etas);

        // settings
        std::vector<source> sources_;
        uint32_t refresh_ms_;
        time::RealTimeClock *rtc_;
        bool running_ = false;

        // transit data
        // map of all routes per stop to sorted ETAs
        std::map<std::string, std::vector<transitRouteETA>> reference_routes;
        // map of all route lines to sorted ETAs
        std::map<std::string, std::vector<const transitRouteETA*>> routes;
        // all ETAs merged and sorted
        std::vector<const transitRouteETA*> allETAs;

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
