#pragma once
#include "esphome/core/component.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/http_request/http_request.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/color.h"
#include <math.h>
#include <map>

namespace esphome {
namespace transit_511 {

struct source {
    std::string url;
    //uint32_t refresh_ms;
};

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
    // style metadata
    bool rail;
    esphome::Color directionColor;
    esphome::Color routeColor;
};


class Transit511 : public Component {
    public:
        void dump_config() override;
        void setup() override;
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
        void set_route_color(std::string route, esphome::Color color) {
            this->route_colors_[route] = color;
        }
        void set_direction_color(std::string direction, esphome::Color color) {
            this->direction_colors_[direction] = color;
        }
        void set_default_route_color(esphome::Color color) {
            this->default_route_color_ = color;
        }
        void set_separator_color(esphome::Color color) {
            this->separator_color_ = color;
        }
        void set_max_eta_ms(uint max_ms) {
            this->max_eta_ms_ = max_ms;
        }

        esphome::Color get_route_color(std::string route);
        esphome::Color get_direction_color(std::string direction);
        esphome::Color get_separator_color() { return this->separator_color_; };

        void refresh(bool force=false);
        bool running() { return this->running_; };

        const std::map<std::string, std::vector<transitRouteETA>> get_reference_routes() { return this->reference_routes; };
        const std::map<std::string, std::vector<const transitRouteETA*>> get_routes() { return this->routes; };
        //const std::vector<const transitRouteETA*> get_ETAs() { return this->allETAs; };

        void debug_print();

        // returns the number of active routes with ETAs before before
        uint get_num_active_routes() { return this->num_active_; };
        //uint get_num_active_routes_now(time_t after, time_t before);
        bool is_route_active(std::string route){ return this->active_[route]; };

    protected:
        void http_response_callback(std::shared_ptr<http_request::HttpContainer> response, std::string & body);
        void http_error_callback();
        http_request::HttpRequestSendAction<> *http_action_;
        size_t max_response_buffer_size_ = 0;
        http_request::HttpRequestComponent * http_;
        wifi::WiFiComponent *wifi_;

        // logic
        void parse_transit_response(std::string body);
        void sortETA();
        void addETAs(std::vector<transitRouteETA> etas);
        void cleanup_route_ETAs();
        void update_active_routes(uint before_ms);

        // settings
        std::vector<source> sources_;
        uint32_t refresh_ms_;
        time::RealTimeClock *rtc_;
        bool running_ = false;
        uint max_eta_ms_ = UINT_MAX;

        // transit data
        // map of all routes per stop to sorted ETAs
        std::map<std::string, std::vector<transitRouteETA>> reference_routes;
        // map of all route lines to sorted ETAs
        std::map<std::string, std::vector<const transitRouteETA*>> routes;
        // all ETAs merged and sorted
        //std::vector<const transitRouteETA*> allETAs;
        std::map<std::string, bool> active_;
        uint num_active_ = 0;

        // colors
        std::map<std::string, esphome::Color> direction_colors_;
        std::map<std::string, esphome::Color> route_colors_;
        esphome::Color default_route_color_, separator_color_;

        // scheduling
        int64_t get_time_ns_();
        void set_next_call_ns_();
        int64_t next_call_ns_{0};
        int64_t last_time_ms_{0};
        uint32_t millis_overflow_counter_{0};
        uint32_t update_active_last_ms{0};
};

void debug_print_tm(tm t);
time_t timeFromJSON(const char *str);

bool etaCmp(const transitRouteETA* a, const transitRouteETA* b);
bool etaCmpRef(const transitRouteETA& a, const transitRouteETA& b);
bool isRail(std::string name);

} // namespace transit_511
} // namespace esphome

// allow accessing timezone offset directly
extern long _timezone;
