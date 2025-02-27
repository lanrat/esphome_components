#include "transit_511.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/base_automation.h"

#include "esphome/components/json/json_util.h"

namespace esphome {
namespace transit_511 {

static const char *const TAG = "transit_511";


void Transit511::setup() {
    //  set action
    this->http_action_ = new http_request::HttpRequestSendAction<>(this->http_);
    this->http_action_->set_method("GET");
    if (this->max_response_buffer_size_ > 0) {
        this->http_action_->set_max_response_buffer_size(this->max_response_buffer_size_);
    }
    this->http_action_->set_capture_response(true);

    // https://github.com/espressif/arduino-esp32/pull/9863
    // NOTE: the current esp-arduino API does not allow overriding this. Setting this header just adds a 2nd value
    // the upstream (511.org) does not parse it correctly, and responds with gzip data
    // attempted using a cloudflare to work around this, but cloudflare always responds with HTTP chunked data which
    //  is also currently broken in ESPHome
    // working solution was to create a cloudflare worker that grabs all the data and responds unchunking it.
    this->http_action_->add_header("Accept-Encoding", "identity"); // disables GZIP

    // HTTP response trigger
    auto http_response_trigger = new http_request::HttpRequestResponseTrigger();
    this->http_action_->register_response_trigger(http_response_trigger);
    auto http_response_trigger_automation = new Automation<std::shared_ptr<http_request::HttpContainer>, std::string &>(http_response_trigger);
    auto *http_response_lambda = new LambdaAction<std::shared_ptr<http_request::HttpContainer>, std::string &>
    ([=](std::shared_ptr<http_request::HttpContainer> response, std::string & body) -> void { this->http_response_callback(response, body); });
    http_response_trigger_automation->add_actions({http_response_lambda});

    // HTTP error trigger
    auto http_error_trigger = new Trigger<>();
    this->http_action_->register_error_trigger(http_error_trigger);
    auto http_error_trigger_automation = new Automation<>(http_error_trigger);
    auto http_error_lambda = new LambdaAction<>([=]() -> void { this->http_error_callback(); });
    http_error_trigger_automation->add_actions({http_error_lambda});
}

void Transit511::http_response_callback(std::shared_ptr<http_request::HttpContainer> response, std::string & body) {
    ESP_LOGD(TAG, "response finished in %dms content length: %d, code: %d", response->duration_ms, response->content_length, response->status_code);

    if (!http_request::is_success(response->status_code)) {
        ESP_LOGE(TAG, "HTTP Error code: %d", response->status_code);
        return;
    }

    this->parse_transit_response(body);
}

void Transit511::http_error_callback() {
    ESP_LOGE(TAG, "HTTP Error!");
}

void Transit511::set_max_response_buffer_size(size_t max_response_buffer_size) {
    this->max_response_buffer_size_ = max_response_buffer_size;

    // set the action's buffer size in case it is being updated dynamically
    if (this->http_action_) {
        this->http_action_->set_max_response_buffer_size(this->max_response_buffer_size_);
    }
}

void Transit511::loop() {
    const uint update_sec = 5;
    if (this->routes.size() > 0) {
        auto ms = millis();
        if ((ms-update_active_last_ms) >= (update_sec * 1000)) {
            update_active_last_ms = ms;
            this->update_active_routes(this->max_eta_ms_);
        }
    }

    this->refresh();
}

void Transit511::refresh(bool force) {
    // check if this should run based on speed
    if (!force) {
        int64_t curr_time_ns = this->get_time_ns_();
        if (curr_time_ns < this->next_call_ns_) {
            return;
        }
    }

    if (!this->http_action_) {
        // not ready yet
        ESP_LOGE(TAG, "ERROR: refresh() called before setup()");
        return;
    }

    if (this->running_) {
        ESP_LOGE(TAG, "ERROR: Already running!");
        return;
    }

    if (!this->wifi_->is_connected()) {
        ESP_LOGD(TAG, "Wifi Not connected, unable to refresh");
        
        // set next time to run
        this->set_next_call_ns_();
        return;
    }

    ESP_LOGD(TAG, "Refreshing Data");
    this->running_ = true;
    for (const auto source : this->sources_) {
        ESP_LOGD(TAG, "Requesting: %s", source.url.c_str());
        this->http_action_->set_url(source.url.c_str());
        this->http_action_->play();
    }

    this->cleanup_route_ETAs();
    
    // set next time to run
    this->set_next_call_ns_();
    this->running_ = false;
}


// get called automatically every second
void Transit511::update_active_routes(uint before_ms) {
    ESPTime esp_now = this->rtc_->now();
    if (!esp_now.is_valid()) {
        return;
    }
    uint new_active = 0;
    time_t now = esp_now.timestamp;
    time_t before_time = now + (before_ms/1000); // ms -> sec
    std::map<std::string, bool> active;
    for (const auto &route : this->routes) {
        for (const auto &eta : route.second) {
            if (eta->ETA >= now && eta->ETA <= before_time) {
                active[route.first] = true;
                new_active++;
                break;
            }
        }
    }

    this->active_.swap(active);
    this->num_active_ = new_active;
}

void Transit511::cleanup_route_ETAs() {
    // Note, this will leave old routes in allETAs until next refresh
    time_t now = this->rtc_->now().timestamp;
    // if a route or source only has ETAs in the past, remove it
    // routes
    for (const auto &route : this->routes) {
        // remove if empty
        if (route.second.empty()) {
            this->routes.erase(route.first);
        }
        // remove route if last ETA is in past
        if (route.second.back()->ETA < now) {
            this->routes.erase(route.first);
        }
    }
}

void Transit511::add_source(std::string url) {
    this->sources_.push_back({url: url});
}

void Transit511::set_wifi(wifi::WiFiComponent *wifi) {
    this->wifi_ = wifi;

    // define trigger
    auto wifi_connect_lambda = new LambdaAction<>([=]() -> void { this->refresh(); });

    // set trigger
    auto wifi_connect_automation = new Automation<>(this->wifi_->get_connect_trigger());
    wifi_connect_automation->add_actions({wifi_connect_lambda});

}

void Transit511::parse_transit_response(std::string body){
    //ESP_LOGD(TAG, "HTTP Response Body len: %d", body.length());

    size_t start = body.find_first_of('{');
    if (start == std::string::npos) {
        // not found
        ESP_LOGE(TAG, "unable to find '{' to start json parsing from string size: %d", body.size());
        if (body.size() > 0) {
            ESP_LOGE(TAG, "body[:100]: %s", body.substr(0, 100));
        }
        return;
    }
    body = body.substr(start);
    //ESP_LOGD(TAG, "HTTP Response Data: %s", body.c_str());

    bool parse_success = esphome::json::parse_json(body, [&](JsonObject root) -> bool {
        //auto TAG = "Transit_JSON";
        ESPTime now = this->rtc_->now();

        /*
        ESP_LOGD(TAG, "PST now [%d]", now.timestamp);
        ESP_LOGD(TAG, "UTC Now [%d]", this->rtc_.utcnow().timestamp);
        ESP_LOGD(TAG, "offset: %d, UTC offset: %d isDST: %d", now.timezone_offset(), this->rtc_.utcnow().timezone_offset(), now.is_dst);
        */

        auto response_ts_str = root["ServiceDelivery"]["StopMonitoringDelivery"]["ResponseTimestamp"].as<const char*>();
        auto response_ts =  timeFromJSON(response_ts_str);
        if (response_ts == -1) {
            ESP_LOGE(TAG, "ResponseTimestamp missing in json");
            return false;
        }
        //ESP_LOGD(TAG, "API Data timestamp: %.19s", ctime(&response_ts));

        JsonArray stopVisits = root["ServiceDelivery"]["StopMonitoringDelivery"]["MonitoredStopVisit"];
        std::vector<transitRouteETA> etas;
        for(const JsonObject& value : stopVisits) {
            // extract vars from json
            auto lineName = value["MonitoredVehicleJourney"]["LineRef"].as<std::string>();
            auto direction = value["MonitoredVehicleJourney"]["DirectionRef"].as<std::string>();
            auto reference = value["MonitoringRef"].as<std::string>();
            auto etaStr = value["MonitoredVehicleJourney"]["MonitoredCall"]["ExpectedArrivalTime"].as<const char*>();
            auto recordedTime = value["RecordedAtTime"].as<const char*>();
    
            if (etaStr == NULL) {
                ESP_LOGE(TAG, "ExpectedArrivalTime missing in json");
                continue;
            }

            if (recordedTime == NULL) {
                ESP_LOGE(TAG, "RecordedAtTime missing in json");
                continue;
            }

            // parse recorded time
            time_t recorded_timestamp = timeFromJSON(recordedTime);
            if (recorded_timestamp == -1) {
                ESP_LOGE(TAG, "timeFromJSON() unable to convert RecordedAtTime from json string: '%s'", recordedTime);
                continue;
            }
            //ESP_LOGD(TAG, "RecordedAtTime: [%d] %.19s", recorded_timestamp, ctime(&recorded_timestamp));
            //ESP_LOGD(TAG, "RecordedAtTime Delta: [%d] ", now.timestamp - recorded_timestamp);

            // parse eta time
            time_t eta_timestamp = timeFromJSON(etaStr);
            if (eta_timestamp == -1) {
                ESP_LOGE(TAG, "timeFromJSON() unable to convert ExpectedArrivalTime from json string: '%s'", etaStr);
                continue;
            }
            double eta_s = difftime(eta_timestamp, now.timestamp);

            bool live = false;
            // if the response TS is less than a single week, it is stale or epoch0 and not live
            if (recorded_timestamp > 0) {
                live = true;
            }

            //ESP_LOGI(TAG, "Line: %s, Direction: %s, live: %d, eta: [%d] eta_min: %.1f", lineName.c_str(), direction.c_str(), live, eta_timestamp, eta_s/60.0);

            auto color = this->get_direction_color(direction);
            if (!live) {
                // make non-live colors darker
                color = color.darken(80);
            }

            // create eta
            transitRouteETA eta = {
                .reference = reference,
                .Name = lineName,
                .Direction = direction,
                .RecordedAtTime = recorded_timestamp,
                .ETA = eta_timestamp,
                .live = live,
                .rail = isRail(lineName),
                .directionColor = color,
                .routeColor = this->get_route_color(lineName),
            };
            etas.push_back(eta);
        }

        if (etas.size() == 0) {
            ESP_LOGW(TAG, "Got %d ETAs", etas.size());
        }

        this->addETAs(etas);
        // return true if we parsed >0 ETAs
        return response_ts > 0;
    });

    if (!parse_success) {
        ESP_LOGE(TAG, "Error Parsing JSON");
    }

    this->debug_print();
}

void Transit511::sortETA() {
    // temp variables to build the new sorted list and swap with the global ones
    //std::vector<const transitRouteETA*> newETA;
    std::map<std::string, std::vector<const transitRouteETA*>> newRoutes;

    // TODO make more efficient with a priority list

    // iterate over all routes
    for (auto const& route_stop : this->reference_routes) {
        // iterate over all ETAs

        for(const auto& eta : route_stop.second) {
            // add to vector of all ETAs
            //newETA.push_back(&eta);
            // add to map per line
            newRoutes[eta.Name].push_back(&eta);
        }
    }
    // TODO do a form of insertion sort.
   // sort(newETA.begin(), newETA.end(), etaCmp);

    // sort all lines (with directions merged)
    for (auto & route : newRoutes) {

        // TODO do a form of insertion sort.
        sort(route.second.begin(), route.second.end(), etaCmp);
        route.second.shrink_to_fit();
    }

    this->routes.swap(newRoutes);
    // this->allETAs.swap(newETA);
    // this->allETAs.shrink_to_fit();
}

void Transit511::addETAs(std::vector<transitRouteETA> etas) {
    if (etas.size() == 0) {
        return;
    }
    // sort ETAs
    sort(etas.begin(), etas.end(), etaCmpRef);

    // check to see if route already exists
    auto ref = etas[0].reference;
    this->reference_routes[ref].swap(etas);
    this->sortETA();
    etas.clear();
}

// this MUST be called after set_max_response_buffer_size
void Transit511::set_http(http_request::HttpRequestComponent * http) {
    this->http_ = http;
}

void Transit511::set_next_call_ns_() {
  auto wait_ms = this->refresh_ms_;
  //ESP_LOGD(TAG, "Waiting for %d ms for next iteration", wait_ms);
  this->next_call_ns_ = (wait_ms * INT64_C(1000000)) + this->get_time_ns_();
}

void Transit511::dump_config() {
  ESP_LOGCONFIG(TAG, "refresh_ms: %d", this->refresh_ms_);
  ESP_LOGCONFIG(TAG, "max_response_buffer_size: %d", this->max_response_buffer_size_);
  for (const auto source : this->sources_) {
    ESP_LOGCONFIG(TAG, "\t URL: %s", source.url.c_str());
  }
  for (const auto color : this->direction_colors_) {
    ESP_LOGCONFIG(TAG, "\t Direction Color: %s (%d,%d,%d)", color.first.c_str(), color.second.red, color.second.green, color.second.blue);
  }
  ESP_LOGCONFIG(TAG, "Default Route Color: (%d,%d,%d)", this->default_route_color_.red, this->default_route_color_.green, this->default_route_color_.blue);
  for (const auto color : this->route_colors_) {
    ESP_LOGCONFIG(TAG, "\t Route Color: %s (%d,%d,%d)", color.first.c_str(), color.second.red, color.second.green, color.second.blue);
  }
  ESP_LOGCONFIG(TAG, "Separator Color: (%d,%d,%d)", this->separator_color_.red, this->separator_color_.green, this->separator_color_.blue);

}

int64_t Transit511::get_time_ns_() {
  int64_t time_ms = millis();
  if (this->last_time_ms_ > time_ms) {
    this->millis_overflow_counter_++;
  }
  this->last_time_ms_ = time_ms;

  return (time_ms + ((int64_t) this->millis_overflow_counter_ << 32)) * INT64_C(1000000);
}

esphome::Color Transit511::get_route_color(std::string route) {
    auto color = this->default_route_color_;
    auto search = route_colors_.find(route);
    if (search != route_colors_.end()) {
        // ESP_LOGD(TAG, "found custom color for %s", route.first.c_str());
        color = route_colors_[route];
    }
    return color;
}

esphome::Color Transit511::get_direction_color(std::string direction) {
    return direction_colors_[direction];
}

void Transit511::debug_print() {
    if (rtc_ == nullptr) {
        ESP_LOGE(TAG, "Error unable to find Time source!");
        return;
    }
    time_t now = rtc_->now().timestamp;

    // print for all stops
    // ESP_LOGD(TAG, "transit stops: %d, size: %d", reference_routes.size(), allETAs.size());
    // for (auto const& route : reference_routes) {
    //     ESP_LOGD(TAG, "transit stop: %s, size: %d", route.first.c_str(), route.second.size());
    //     // iterate over all ETAs for route
    //     for(const auto& eta : route.second) {
    //         double eta_s = difftime(eta.ETA, now);
    //         ESP_LOGD(TAG, "Stop ETA: %s:%s ETA: [%d] %.1fmin live: %s", eta.Name.c_str(), eta.Direction.c_str(), eta.ETA, eta_s/60.0, eta.live ? "true" : "false");
    //     }
    // }

    // // print from allETAs
    // ESP_LOGD(TAG, "allETAs..");
    // for(const auto& eta : allETAs) {
    //     double eta_s = difftime(eta->ETA, now);
    //     ESP_LOGD(TAG, "ETA: %s:%s ETA: [%d] %.1fmin live: %s", eta->Name.c_str(), eta->Direction.c_str(), eta->ETA, eta_s/60.0, eta->live ? "true" : "false");
    // }

    // print from all stops
    ESP_LOGD(TAG, "routes, len: %d",  this->routes.size());
    for(const auto& route : this->routes) {
        ESP_LOGD(TAG, "route: %s len: %d", route.first.c_str(), route.second.size());
        for(const auto& eta : route.second) {
            double eta_s = difftime(eta->ETA, now);
            ESP_LOGD(TAG, "Route: %s:%s ETA: [%d] %.1fmin live: %s", eta->Name.c_str(), eta->Direction.c_str(), eta->ETA, eta_s/60.0, eta->live ? "true" : "false");
        }
    }

    ESP_LOGD(TAG, "active routes, len: %d, num_active: %d",  this->active_.size(), this->num_active_);
    for(const auto& line : this->active_) {
        ESP_LOGD(TAG, "line: %s active: %d", line.first.c_str(), line.second);
    }
}

void debug_print_tm(tm t) {
    ESP_LOGD("debug_print_tm","TM: tm_sec[%d] tm_min[%d] tm_hour[%d] tm_mday[%d] tm_mon[%d] tm_year[%d] tm_wday[%d] tm_yday[%d] tm_isdst[%d]",
                       t.tm_sec,  t.tm_min,  t.tm_hour,  t.tm_mday,  t.tm_mon,  t.tm_year,  t.tm_wday,  t.tm_yday,  t.tm_isdst);
}

time_t timeFromJSON(const char *str) {
    auto TAG = "timeFromJSON";
    if (str == NULL) {
        ESP_LOGE(TAG, "str is NULL");
        return -1;
    }

    // parse time
    struct tm time_info = {0}; // initialize to all 0
    if (strptime(str, "%FT%TZ", &time_info) == NULL) {
        ESP_LOGE(TAG, "strptime() unable to parse json time string: '%s'", str);
        return -1;
    }
    //debug_print_tm(time_info);
    time_t timestamp = mktime(&time_info);
    //time_t timestamp = timegm(&time_info);
    if (timestamp == -1) {
        ESP_LOGE(TAG, "mktime() unable to convert tm from json string: '%s'", str);
        return -1;
    }
    // mktime returns a timestamp relative to the current timezone. To get UTC, subtract from global timezone variable
    // would be better to use timegm instead of mktime, which will always return UTC time, but that appears to not be available
    timestamp -= _timezone;
    //debug_print_tm(time_info);

    //ESP_LOGD(TAG, "converted json time to unix: %s -> %d", str, timestamp);
    return timestamp;
}

bool etaCmp(const transitRouteETA* a, const transitRouteETA* b) {
    return a->ETA < b->ETA;
}

bool etaCmpRef(const transitRouteETA& a, const transitRouteETA& b) {
    return etaCmp(&a, &b);
}

// returns true if A-Z
bool isRail(std::string name) {
  if (name.length() == 1) {
    char c = name.c_str()[0];
    if (c >= 'A' && c <= 'Z') {
      return true;
    }
  }
  return false;
}

} // namespace transit_511
} // namespace esphome
