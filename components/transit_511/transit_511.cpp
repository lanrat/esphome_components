#include "transit_511.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/components/json/json_util.h"

namespace esphome {
namespace transit_511 {

static const char *const TAG = "transit_511";

void Transit511::setup() {
}

void Transit511::loop() {
    // check if this should run based on speed
    int64_t curr_time_ns = this->get_time_ns_();
    if (curr_time_ns < this->next_call_ns_) {
      return;
    }
    // set next time to run
    this->set_next_call_ns_();

    // TODO do work
    ESP_LOGE(TAG, "Refreshing Data");
}

void Transit511::add_source(std::string url) {
    this->sources_.push_back({url: url});
}

void Transit511::parse_transit_response(std::string body){
    //yield(); // allow other tasks to run

    auto TAG = "Transit";
    //ESP_LOGD(TAG, "HTTP Response Body len: %d", body.length());

    size_t start = body.find_first_of('{');
    if (start == std::string::npos) {
        // not found
        ESP_LOGE(TAG, "unable to find '{' to start json parsing");
        return;
    }
    body = body.substr(start);
    //ESP_LOGD(TAG, "HTTP Response Data: %s", body.c_str());

    bool parse_success = esphome::json::parse_json(body, [&](JsonObject root) -> bool {
        //auto TAG = "Transit_JSON";
        ESPTime now = this->rtc_->now();

        /*
        ESP_LOGD(TAG, "PST now [%d]", now.timestamp);
        ESP_LOGD(TAG, "UTC Now [%d]", id(sntp_time).utcnow().timestamp);
        ESP_LOGD(TAG, "offset: %d, UTC offset: %d isDST: %d", now.timezone_offset(), id(sntp_time).utcnow().timezone_offset(), now.is_dst);
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
            
            // create eta
            transitRouteETA eta = {
                .reference = reference,
                .Name = lineName,
                .Direction = direction,
                .RecordedAtTime = recorded_timestamp,
                .ETA = eta_timestamp,
                .live = live
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

    this->debugPrint();
}

void Transit511::sortETA() {
    //yield(); // allow other tasks to run

    // temp variables to build the new sorted list and swap with the global ones
    std::vector<const transitRouteETA*> newETA;
    std::map<std::string, std::vector<const transitRouteETA*>> newRoutes;

    // TODO make more efficient with a priority list

    // iterate over all routes
    for (auto const& route_stop : this->reference_routes) {
        // iterate over all ETAs

        for(const auto& eta : route_stop.second) {
            // add to vector of all ETAs
            newETA.push_back(&eta);
            // add to map per line
            newRoutes[eta.Name].push_back(&eta);
        }
    }
    // TODO do a form of insertion sort.
    sort(newETA.begin(), newETA.end(), etaCmp);

    // sort all lines (with directions merged)
    for (auto & route : newRoutes) {
        //yield(); // allow other tasks to run

        // TODO do a form of insertion sort.
        sort(route.second.begin(), route.second.end(), etaCmp);
        route.second.shrink_to_fit();
    }

    this->routes.swap(newRoutes);
    this->allETAs.swap(newETA);
    this->allETAs.shrink_to_fit();
}

void Transit511::addETAs(std::vector<transitRouteETA> etas) {
    if (etas.size() == 0) {
        return;
    }
    // sort ETAs
    sort(etas.begin(), etas.end(), etaCmpRef);

    // check to see if route already exists
    auto ref = etas[0].reference;
    // TODO not sure if I may need to initialize this first
    this->reference_routes[ref].swap(etas);
    this->sortETA();
    // TODO possible race condition here....
    etas.clear();
}



void Transit511::set_next_call_ns_() {
  auto wait_ms = this->refresh_ms_;
  //ESP_LOGD(TAG, "Waiting for %d ms for next iteration", wait_ms);
  this->next_call_ns_ = (wait_ms * INT64_C(1000000)) + this->get_time_ns_();
}

void Transit511::dump_config() {
  ESP_LOGCONFIG(TAG, "refresh_ms: %d", this->refresh_ms_);
  for ( const auto source : this->sources_) {
    ESP_LOGCONFIG(TAG, "\t URL: %s", source.url.c_str());
  }
}

int64_t Transit511::get_time_ns_() {
  int64_t time_ms = millis();
  if (this->last_time_ms_ > time_ms) {
    this->millis_overflow_counter_++;
  }
  this->last_time_ms_ = time_ms;

  return (time_ms + ((int64_t) this->millis_overflow_counter_ << 32)) * INT64_C(1000000);
}


void Transit511::debugPrint() {
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
    ESP_LOGD(TAG, "routes, len: %d",  routes.size());
    for(const auto& route : routes) {
        ESP_LOGD(TAG, "route: %s len: %d", route.first.c_str(), route.second.size());
        for(const auto& eta : route.second) {
            double eta_s = difftime(eta->ETA, now);
            ESP_LOGD(TAG, "Route: %s:%s ETA: [%d] %.1fmin live: %s", eta->Name.c_str(), eta->Direction.c_str(), eta->ETA, eta_s/60.0, eta->live ? "true" : "false");
        }
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

} // namespace game_of_life
} // namespace esphome
