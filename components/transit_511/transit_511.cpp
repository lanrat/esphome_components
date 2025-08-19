#include "transit_511.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/base_automation.h"

#include "esphome/components/json/json_util.h"
#include <WiFi.h>

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
    
    // Set HTTP timeout to prevent watchdog timeouts
    // CRITICAL: HTTPClient has blocking delays that cause watchdog timeouts
    // We reduce timeout to minimize impact but can't fully fix the library's blocking behavior
    this->http_->set_timeout(3000);  // Further reduced to 3s to minimize blocking
    
    // WARNING: ESP32 Arduino HTTPClient contains blocking delay(10) calls in handleHeaderResponse
    // that can accumulate and trigger watchdog timeouts. Consider using async HTTP if crashes persist.

    // https://github.com/espressif/arduino-esp32/pull/9863
    // NOTE: the current esp-arduino API does not allow overriding this. Setting this header just adds a 2nd value
    // the upstream (511.org) does not parse it correctly, and responds with gzip data
    // attempted using a cloudflare to work around this, but cloudflare always responds with HTTP chunked data which
    //  is also currently broken in ESPHome
    // working solution was to create a cloudflare worker that grabs all the data and responds unchunking it.
    this->http_action_->add_request_header("Accept-Encoding", "identity"); // disables GZIP

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
    App.feed_wdt(); // feed watchdog
    ESP_LOGD(TAG, "response finished in %dms content length: %d, code: %d", response->duration_ms, response->content_length, response->status_code);

    if (!http_request::is_success(response->status_code)) {
        ESP_LOGE(TAG, "HTTP Error code: %d", response->status_code);
        this->consecutive_errors_++;
        this->last_error_ms_ = millis();
    } else {
        this->parse_transit_response(body);
        this->consecutive_errors_ = 0; // Reset error counter on success
    }
    
    // Decrement pending requests and clear running flag when all complete
    if (this->pending_requests_ > 0) {
        this->pending_requests_--;
        if (this->pending_requests_ == 0) {
            this->running_ = false;
            this->current_request_index_ = 0;  // Reset for next refresh
            this->request_start_ms_ = 0;  // Reset request timer
            ESP_LOGD(TAG, "All HTTP requests completed");
        }
    }
}

void Transit511::http_error_callback() {
    App.feed_wdt(); // feed watchdog
    ESP_LOGE(TAG, "HTTP Error!");
    this->consecutive_errors_++;
    this->last_error_ms_ = millis();
    
    // Also handle error case for pending request tracking
    if (this->pending_requests_ > 0) {
        this->pending_requests_--;
        if (this->pending_requests_ == 0) {
            this->running_ = false;
            this->current_request_index_ = 0;  // Reset for next refresh
            this->request_start_ms_ = 0;  // Reset request timer
            ESP_LOGD(TAG, "All HTTP requests completed (with errors)");
        }
    }
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

    // Check for stuck requests (safety timeout)
    if (this->running_ && this->request_start_ms_ > 0) {
        const uint32_t REQUEST_TIMEOUT_MS = 30000; // 30 second overall timeout
        if (millis() - this->request_start_ms_ > REQUEST_TIMEOUT_MS) {
            ESP_LOGE(TAG, "Request timeout exceeded, resetting state");
            this->running_ = false;
            this->pending_requests_ = 0;
            this->current_request_index_ = 0;
            this->request_start_ms_ = 0;
            this->consecutive_errors_++;
            this->last_error_ms_ = millis();
            return;
        }
    }

    // Add delay between HTTP requests to prevent blocking
    static uint32_t last_request_ms = 0;
    // INCREASED DELAY: HTTPClient's blocking delays accumulate, need more spacing
    const uint32_t REQUEST_DELAY_MS = 500; // Increased from 100ms to 500ms to prevent watchdog
    
    // Process one HTTP request per loop() call if we have pending requests
    if (this->running_ && this->current_request_index_ < this->sources_.size()) {
        // Ensure minimum delay between requests
        uint32_t now_ms = millis();
        if (now_ms - last_request_ms < REQUEST_DELAY_MS) {
            App.feed_wdt(); // Feed watchdog while waiting
            return; // Exit to let system breathe
        }
        
        // Check WiFi is still connected before attempting request
        // This prevents blocking on connection attempts when WiFi is down
        if (!this->wifi_->is_connected()) {
            ESP_LOGW(TAG, "WiFi disconnected during request sequence, aborting");
            this->running_ = false;
            this->pending_requests_ = 0;
            this->current_request_index_ = 0;
            this->request_start_ms_ = 0;
            this->wifi_connected_ms_ = 0;  // Reset WiFi connection time
            this->consecutive_errors_++;
            this->last_error_ms_ = now_ms;
            return;
        }
        
        // Check WiFi signal strength to avoid attempts on weak connections
        int8_t rssi = WiFi.RSSI();
        if (rssi < -85) {  // Very weak signal
            ESP_LOGW(TAG, "WiFi signal too weak (RSSI: %d dBm), delaying request", rssi);
            App.feed_wdt();
            return;  // Skip this cycle, will retry next loop
        }
        
        const auto& source = this->sources_[this->current_request_index_];
        ESP_LOGD(TAG, "Requesting (%d/%d): %s", this->current_request_index_ + 1, this->sources_.size(), source.url.c_str());
        
        // Feed watchdog multiple times during request setup
        App.feed_wdt();
        yield();  // Allow other tasks to run before blocking operations
        
        // Track request start time for timeout
        if (this->request_start_ms_ == 0) {
            this->request_start_ms_ = now_ms;
        }
        
        // Pre-feed watchdog before potentially blocking HTTP operations
        App.feed_wdt();
        
        this->http_action_->set_url(source.url.c_str());
        
        // CRITICAL: The play() call triggers HTTPClient which has blocking delays
        // We can't prevent the blocking, but we can ensure watchdog is well-fed before
        App.feed_wdt();
        yield();
        
        this->http_action_->play();
        
        // Immediately yield after starting request to let system process
        yield();
        App.feed_wdt();
        
        this->current_request_index_++;
        last_request_ms = now_ms;
        
        // Feed watchdog after initiating request
        App.feed_wdt();
        
        return; // Exit loop() to let system breathe
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

    // Implement exponential backoff for consecutive errors
    if (this->consecutive_errors_ > 0) {
        const uint32_t MAX_ERRORS = 10;  // Stop trying after 10 consecutive errors
        const uint32_t MAX_BACKOFF_MS = 300000; // 5 minutes max
        
        // If too many consecutive errors, wait longer before retrying
        if (this->consecutive_errors_ >= MAX_ERRORS) {
            const uint32_t LONG_RETRY_MS = 600000; // 10 minutes for recovery
            if (millis() - this->last_error_ms_ < LONG_RETRY_MS) {
                ESP_LOGD(TAG, "Max errors reached (%d), waiting %dms before retry", 
                         this->consecutive_errors_, LONG_RETRY_MS - (millis() - this->last_error_ms_));
                return;
            }
            // Reset error count after long wait
            this->consecutive_errors_ = 0;
        } else {
            uint32_t backoff_ms = std::min((uint32_t)(1000 * pow(2, this->consecutive_errors_)), MAX_BACKOFF_MS);
            if (millis() - this->last_error_ms_ < backoff_ms) {
                ESP_LOGD(TAG, "In error backoff period (%d errors, %dms remaining)", 
                         this->consecutive_errors_, backoff_ms - (millis() - this->last_error_ms_));
                return;
            }
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

    // Wait 2 seconds after WiFi connects before making requests
    // This allows the connection to stabilize and prevents issues
    const uint32_t WIFI_STABILIZATION_MS = 2000;
    if (this->wifi_connected_ms_ > 0) {
        uint32_t time_since_connect = millis() - this->wifi_connected_ms_;
        if (time_since_connect < WIFI_STABILIZATION_MS) {
            //ESP_LOGD(TAG, "Waiting for WiFi to stabilize (%dms/%dms)", 
             //        time_since_connect, WIFI_STABILIZATION_MS);
            return;  // Try again next loop
        }
    }

    ESP_LOGD(TAG, "Refreshing Data");
    this->running_ = true;
    this->pending_requests_ = this->sources_.size();
    this->current_request_index_ = 0;  // Reset request index
    
    // Don't make requests here - let loop() handle them one at a time
    // This prevents blocking the main loop for too long
    
    this->cleanup_route_ETAs();
    
    // set next time to run
    this->set_next_call_ns_();
    
    // Note: running_ will be set to false when all HTTP requests complete
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
    
    // Use safe iteration pattern to avoid iterator invalidation
    auto it = this->routes.begin();
    while (it != this->routes.end()) {
        bool should_remove = false;
        
        // remove if empty
        if (it->second.empty()) {
            should_remove = true;
        }
        // remove route if last ETA is in past
        else if (it->second.back()->ETA < now) {
            should_remove = true;
        }
        
        if (should_remove) {
            it = this->routes.erase(it);  // erase returns next valid iterator
        } else {
            ++it;
        }
    }
}

void Transit511::add_source(std::string url) {
    this->sources_.push_back({url: url});
}

void Transit511::add_route_filter(std::string route) {
    this->route_filter_.insert(route);
}

bool Transit511::is_route_filtered(const std::string& route_name) {
    // If no filter is set, allow all routes
    if (this->route_filter_.empty()) {
        return false;
    }
    // Return true if route should be filtered out (not in the filter list)
    return this->route_filter_.find(route_name) == this->route_filter_.end();
}

void Transit511::set_wifi(wifi::WiFiComponent *wifi) {
    this->wifi_ = wifi;

    // define trigger
    auto wifi_connect_lambda = new LambdaAction<>([=]() -> void { 
        // Track when WiFi connects and wait before making requests
        this->wifi_connected_ms_ = millis();
        ESP_LOGD(TAG, "WiFi connected, will wait 3s before making requests");
        // Don't immediately refresh - let the connection stabilize
    });

    // set trigger
    auto wifi_connect_automation = new Automation<>(this->wifi_->get_connect_trigger());
    wifi_connect_automation->add_actions({wifi_connect_lambda});
}

void Transit511::parse_transit_response(std::string body){
    // Feed watchdog at start of potentially long parsing operation
    App.feed_wdt();
    //ESP_LOGD(TAG, "HTTP Response Body len: %d", body.length());

    // Check available heap before processing
    size_t free_heap = ESP.getFreeHeap();
    const size_t MIN_FREE_HEAP = 20000; // Require at least 20KB free
    if (free_heap < MIN_FREE_HEAP) {
        ESP_LOGE(TAG, "Low memory: %zu bytes free (minimum: %zu)", free_heap, MIN_FREE_HEAP);
        return;
    }

    // Validate response size to prevent memory exhaustion
    const size_t MAX_RESPONSE_SIZE = 1024 * 1024; // 1MB limit
    if (body.size() > MAX_RESPONSE_SIZE) {
        ESP_LOGE(TAG, "Response too large: %zu bytes (limit: %zu)", body.size(), MAX_RESPONSE_SIZE);
        return;
    }
    
    if (body.empty()) {
        ESP_LOGE(TAG, "Empty response body");
        return;
    }

    size_t start = body.find_first_of('{');
    if (start == std::string::npos) {
        // not found
        ESP_LOGE(TAG, "unable to find '{' to start json parsing from string size: %d", body.size());
        if (body.size() > 0) {
            // Safe substring with bounds checking
            size_t preview_len = std::min(body.size(), static_cast<size_t>(100));
            ESP_LOGE(TAG, "body[:100]: %.*s", static_cast<int>(preview_len), body.c_str());
        }
        return;
    }
    body = body.substr(start);
    //ESP_LOGD(TAG, "HTTP Response Data: %s", body.c_str());

    bool parse_success = esphome::json::parse_json(body, [&](JsonObject root) -> bool {
        //auto TAG = "Transit_JSON";
        App.feed_wdt(); // Feed watchdog before JSON parsing
        
        // Add yield to allow other tasks to run during parsing
        yield();
        
        ESPTime now = this->rtc_->now();

        /*
        ESP_LOGD(TAG, "PST now [%d]", now.timestamp);
        ESP_LOGD(TAG, "UTC Now [%d]", this->rtc_.utcnow().timestamp);
        ESP_LOGD(TAG, "offset: %d, UTC offset: %d isDST: %d", now.timezone_offset(), this->rtc_.utcnow().timezone_offset(), now.is_dst);
        */

        auto response_ts_str = root["ServiceDelivery"]["StopMonitoringDelivery"]["ResponseTimestamp"].as<const char*>();
        if (response_ts_str == nullptr) {
            ESP_LOGE(TAG, "ResponseTimestamp field missing from json");
            return false;
        }
        auto response_ts =  timeFromJSON(response_ts_str);
        if (response_ts == -1) {
            ESP_LOGE(TAG, "ResponseTimestamp invalid in json: '%s'", response_ts_str);
            return false;
        }
        //ESP_LOGD(TAG, "API Data timestamp: %.19s", ctime(&response_ts));

        JsonArray stopVisits = root["ServiceDelivery"]["StopMonitoringDelivery"]["MonitoredStopVisit"];
        if (stopVisits.isNull()) {
            ESP_LOGE(TAG, "MonitoredStopVisit array missing from json");
            return false;
        }
        
        std::vector<transitRouteETA> etas;
        int iteration_count = 0;
        const size_t MAX_ETAS = 100; // Limit total ETAs to prevent memory exhaustion
        
        for(const JsonObject& value : stopVisits) {
            App.feed_wdt(); // feed watchdog
            
            // Limit total number of ETAs
            if (etas.size() >= MAX_ETAS) {
                ESP_LOGW(TAG, "Reached maximum ETA limit (%zu), skipping remaining", MAX_ETAS);
                break;
            }
            
            // Yield periodically to prevent blocking
            if (++iteration_count % 5 == 0) {
                yield();
            }
            // extract vars from json
            auto lineName = value["MonitoredVehicleJourney"]["LineRef"].as<std::string>();
            auto direction = value["MonitoredVehicleJourney"]["DirectionRef"].as<std::string>();
            auto reference = value["MonitoringRef"].as<std::string>();
            auto etaStr = value["MonitoredVehicleJourney"]["MonitoredCall"]["ExpectedArrivalTime"].as<const char*>();
            auto recordedTime = value["RecordedAtTime"].as<const char*>();
    
            // Validate required fields
            if (lineName.empty()) {
                ESP_LOGE(TAG, "LineRef missing in json");
                continue;
            }
            if (direction.empty()) {
                ESP_LOGE(TAG, "DirectionRef missing in json");
                continue;
            }
            if (reference.empty()) {
                ESP_LOGE(TAG, "MonitoringRef missing in json");
                continue;
            }
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

            // Skip this route if it's filtered out
            if (this->is_route_filtered(lineName)) {
                ESP_LOGE(TAG, "Filtering out Route: '%s'", lineName);
                continue;
            }

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
                .ResponseTimestamp = response_ts,
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
    App.feed_wdt(); // Feed watchdog at start of sort
    // temp variables to build the new sorted list and swap with the global ones
    //std::vector<const transitRouteETA*> newETA;
    std::map<std::string, std::vector<const transitRouteETA*>> newRoutes;

    // TODO make more efficient with a priority list

    // iterate over all routes
    int route_count = 0;
    for (auto const& route_stop : this->reference_routes) {
        App.feed_wdt(); // feed watchdog
        
        // Yield periodically to prevent blocking
        if (++route_count % 3 == 0) {
            yield();
        }
        
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
    route_count = 0;
    for (auto & route : newRoutes) {
        App.feed_wdt(); // feed watchdog
        
        // Yield periodically during sorting
        if (++route_count % 2 == 0) {
            yield();
        }
        
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
  if (!this->route_filter_.empty()) {
    ESP_LOGCONFIG(TAG, "Route Filter enabled (%d routes):", this->route_filter_.size());
    for (const auto& route : this->route_filter_) {
      ESP_LOGCONFIG(TAG, "\t Filtered Route: %s", route.c_str());
    }
  } else {
    ESP_LOGCONFIG(TAG, "Route Filter: disabled (showing all routes)");
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
