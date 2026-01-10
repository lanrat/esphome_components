#include "transit_511.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/core/base_automation.h"

#include "esphome/components/json/json_util.h"

// ESP-IDF HTTP client for async requests
#include "esp_http_client.h"

namespace esphome {
namespace transit_511 {

static const char *const TAG = "transit_511";

// Stack size for HTTP task (needs enough for HTTP client + TLS)
static const uint32_t HTTP_TASK_STACK_SIZE = 8192;
// Queue sizes
static const uint32_t REQUEST_QUEUE_SIZE = 4;
static const uint32_t RESPONSE_QUEUE_SIZE = 4;


void Transit511::setup() {
    // Create request and response queues
    this->request_queue_ = xQueueCreate(REQUEST_QUEUE_SIZE, sizeof(HttpRequest));
    this->response_queue_ = xQueueCreate(RESPONSE_QUEUE_SIZE, sizeof(HttpResponse));

    if (this->request_queue_ == nullptr || this->response_queue_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create HTTP queues");
        return;
    }

    // Create background HTTP task
    BaseType_t result = xTaskCreatePinnedToCore(
        Transit511::http_task,      // Task function
        "transit_http",             // Task name
        HTTP_TASK_STACK_SIZE,       // Stack size
        this,                       // Parameter (this pointer)
        1,                          // Priority (low, background task)
        &this->http_task_handle_,   // Task handle
        1                           // Core 1 (keep core 0 for main loop)
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create HTTP task");
        return;
    }

    ESP_LOGI(TAG, "Background HTTP task started");
}

// Background HTTP task - runs on core 1, performs blocking HTTP requests
void Transit511::http_task(void *arg) {
    Transit511 *self = static_cast<Transit511 *>(arg);
    HttpRequest request;

    ESP_LOGD(TAG, "HTTP task running");

    while (true) {
        // Wait for a request (blocks until one is available)
        if (xQueueReceive(self->request_queue_, &request, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        ESP_LOGD(TAG, "HTTP task processing request: %s", request.url);
        uint32_t start_ms = millis();

        // Prepare response
        HttpResponse response = {
            .success = false,
            .status_code = 0,
            .duration_ms = 0,
            .body = nullptr,
            .body_len = 0
        };

        // Configure HTTP client
        esp_http_client_config_t config = {};
        config.url = request.url;
        config.timeout_ms = self->http_timeout_ms_;
        config.buffer_size = 4096;
        config.buffer_size_tx = 1024;
        // Allow HTTPS without certificate verification (insecure but useful for testing/proxies)
        config.skip_cert_common_name_check = true;
        config.crt_bundle_attach = nullptr;  // Don't use certificate bundle

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == nullptr) {
            ESP_LOGE(TAG, "Failed to initialize HTTP client");
            response.duration_ms = millis() - start_ms;
            xQueueSend(self->response_queue_, &response, portMAX_DELAY);
            continue;
        }

        // Set headers
        esp_http_client_set_header(client, "Accept-Encoding", "identity");

        // Open connection and send request
        esp_err_t err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
            esp_http_client_cleanup(client);
            response.duration_ms = millis() - start_ms;
            xQueueSend(self->response_queue_, &response, portMAX_DELAY);
            continue;
        }

        // Fetch headers
        int content_length = esp_http_client_fetch_headers(client);
        response.status_code = esp_http_client_get_status_code(client);

        ESP_LOGD(TAG, "HTTP status: %d, content_length: %d", response.status_code, content_length);

        // Read body
        if (response.status_code >= 200 && response.status_code < 300) {
            // Determine buffer size
            size_t buffer_size = (content_length > 0) ? content_length : 32768;
            if (request.max_response_size > 0 && buffer_size > request.max_response_size) {
                buffer_size = request.max_response_size;
            }

            response.body = (char *)malloc(buffer_size + 1);
            if (response.body != nullptr) {
                size_t total_read = 0;
                int read_len;

                while (total_read < buffer_size) {
                    read_len = esp_http_client_read(client, response.body + total_read, buffer_size - total_read);
                    if (read_len <= 0) {
                        break;
                    }
                    total_read += read_len;
                }

                response.body[total_read] = '\0';
                response.body_len = total_read;
                response.success = true;
                ESP_LOGD(TAG, "HTTP read %zu bytes", total_read);
            } else {
                ESP_LOGE(TAG, "Failed to allocate response buffer");
            }
        }

        esp_http_client_close(client);
        esp_http_client_cleanup(client);

        response.duration_ms = millis() - start_ms;
        ESP_LOGD(TAG, "HTTP request completed in %dms", response.duration_ms);

        // Send response back to main thread
        xQueueSend(self->response_queue_, &response, portMAX_DELAY);
    }
}

// Process HTTP response on main thread
void Transit511::process_http_response(const HttpResponse &response) {
    ESP_LOGD(TAG, "Processing response: success=%d, status=%d, duration=%dms, body_len=%zu",
             response.success, response.status_code, response.duration_ms, response.body_len);

    if (!response.success || response.status_code < 200 || response.status_code >= 300) {
        ESP_LOGE(TAG, "HTTP Error: success=%d, status=%d", response.success, response.status_code);
        this->consecutive_errors_++;
        this->last_error_ms_ = millis();
    } else if (response.body != nullptr && response.body_len > 0) {
        std::string body(response.body, response.body_len);
        this->parse_transit_response(body);
        this->consecutive_errors_ = 0;
    }

    // Free the body buffer (allocated in task)
    if (response.body != nullptr) {
        free(response.body);
    }

    // Track pending requests
    if (this->pending_requests_ > 0) {
        this->pending_requests_--;
        if (this->pending_requests_ == 0) {
            this->running_ = false;
            this->current_request_index_ = 0;
            this->request_start_ms_ = 0;
            ESP_LOGD(TAG, "All HTTP requests completed");
        }
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

    // Poll for HTTP responses from background task (non-blocking)
    if (this->response_queue_ != nullptr) {
        HttpResponse response;
        while (xQueueReceive(this->response_queue_, &response, 0) == pdTRUE) {
            this->process_http_response(response);
        }
    }

    // Check for stuck requests (safety timeout)
    if (this->running_ && this->request_start_ms_ > 0) {
        const uint32_t REQUEST_TIMEOUT_MS = 60000; // 60 second overall timeout (longer since async)
        if (millis() - this->request_start_ms_ > REQUEST_TIMEOUT_MS) {
            ESP_LOGE(TAG, "Request timeout exceeded, resetting state");
            this->running_ = false;
            this->pending_requests_ = 0;
            this->current_request_index_ = 0;
            this->request_start_ms_ = 0;
            this->consecutive_errors_++;
            this->last_error_ms_ = millis();

            // Drain any stale responses from the queue to prevent counter desync
            HttpResponse stale_response;
            while (xQueueReceive(this->response_queue_, &stale_response, 0) == pdTRUE) {
                ESP_LOGW(TAG, "Discarding stale response after timeout");
                if (stale_response.body != nullptr) {
                    free(stale_response.body);
                }
            }
            return;
        }
    }

    // Queue HTTP requests if we have pending ones to send
    if (this->running_ && this->current_request_index_ < this->sources_.size()) {
        // Check WiFi is still connected before attempting request
        if (!this->wifi_->is_connected()) {
            ESP_LOGW(TAG, "WiFi disconnected during request sequence, aborting");
            this->running_ = false;
            this->pending_requests_ = 0;
            this->current_request_index_ = 0;
            this->request_start_ms_ = 0;
            this->wifi_connected_ms_ = 0;
            this->consecutive_errors_++;
            this->last_error_ms_ = millis();
            return;
        }

        // Check WiFi signal strength to avoid attempts on weak connections
        int8_t rssi = this->wifi_->wifi_rssi();
        if (rssi < -85) {
            ESP_LOGW(TAG, "WiFi signal too weak (RSSI: %d dBm), delaying request", rssi);
            return;
        }

        // Queue all remaining requests to the background task
        while (this->current_request_index_ < this->sources_.size()) {
            const auto& source = this->sources_[this->current_request_index_];

            HttpRequest request = {};
            strncpy(request.url, source.url.c_str(), sizeof(request.url) - 1);
            request.max_response_size = this->max_response_buffer_size_;

            ESP_LOGD(TAG, "Queuing request (%zu/%zu): %s",
                     this->current_request_index_ + 1, this->sources_.size(), request.url);

            // Try to queue (non-blocking check)
            if (xQueueSend(this->request_queue_, &request, 0) != pdTRUE) {
                // Queue full, try again next loop
                ESP_LOGD(TAG, "Request queue full, will retry");
                break;
            }

            // Track request start time for timeout
            if (this->request_start_ms_ == 0) {
                this->request_start_ms_ = millis();
            }

            this->current_request_index_++;
        }
        return;
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
                return;  // Silently wait during backoff
            }
            // Reset error count after long wait
            ESP_LOGI(TAG, "Backoff complete after %d errors, retrying", this->consecutive_errors_);
            this->consecutive_errors_ = 0;
        } else {
            uint32_t backoff_ms = std::min((uint32_t)(1000 * pow(2, this->consecutive_errors_)), MAX_BACKOFF_MS);
            if (millis() - this->last_error_ms_ < backoff_ms) {
                return;  // Silently wait during backoff
            }
        }
    }

    if (this->request_queue_ == nullptr) {
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
    auto wifi_connect_lambda = new LambdaAction<>([this]() -> void {
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
    if (body.empty()) {
        ESP_LOGE(TAG, "Empty response body");
        return;
    }

    // Validate response size
    const size_t MAX_RESPONSE_SIZE = 1024 * 1024; // 1MB limit
    if (body.size() > MAX_RESPONSE_SIZE) {
        ESP_LOGE(TAG, "Response too large: %zu bytes (limit: %zu)", body.size(), MAX_RESPONSE_SIZE);
        return;
    }

    size_t start = body.find_first_of('{');
    if (start == std::string::npos) {
        ESP_LOGE(TAG, "Unable to find '{' to start json parsing from string size: %zu", body.size());
        if (body.size() > 0) {
            size_t preview_len = std::min(body.size(), static_cast<size_t>(100));
            ESP_LOGE(TAG, "body[:100]: %.*s", static_cast<int>(preview_len), body.c_str());
        }
        return;
    }
    body = body.substr(start);

    bool parse_success = esphome::json::parse_json(body, [&](JsonObject root) -> bool {
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
        const size_t MAX_ETAS = 100; // Limit total ETAs to prevent memory exhaustion

        for(const JsonObject& value : stopVisits) {
            if (etas.size() >= MAX_ETAS) {
                ESP_LOGW(TAG, "Reached maximum ETA limit (%zu), skipping remaining", MAX_ETAS);
                break;
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
    std::map<std::string, std::vector<const transitRouteETA*>> newRoutes;

    // Build map of routes to ETAs
    for (auto const& route_stop : this->reference_routes) {
        for(const auto& eta : route_stop.second) {
            newRoutes[eta.Name].push_back(&eta);
        }
    }

    // Sort all lines (with directions merged)
    for (auto & route : newRoutes) {
        sort(route.second.begin(), route.second.end(), etaCmp);
        route.second.shrink_to_fit();
    }

    this->routes.swap(newRoutes);
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
