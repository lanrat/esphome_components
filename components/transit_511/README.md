# Transit 511.org

A custom ESPHome component for retrieving real-time transit ETAs (Estimated Time of Arrival) from the 511.org API. This component is designed for displaying transit information on LED matrix displays or other ESPHome-compatible devices.

## Features

- **Real-time transit data** from 511.org APIs
- **Multiple data sources** support for comprehensive coverage
- **Route filtering** to show only specific transit lines
- **Configurable colors** for routes and directions
- **Active route tracking** with time-based filtering
- **Automatic data cleanup** of expired ETAs
- **Built-in error handling** and retry logic

## Dependencies

This component requires the following ESPHome components:

- `time` - For timestamp handling
- `http_request` - For API communication  
- `wifi` - For network connectivity

## Configuration

### Basic Configuration

```yaml
# Required dependencies
time:
  - platform: sntp
    id: sntp_time
    timezone: America/Los_Angeles

http_request:
  verify_ssl: false
  timeout: 3s
  watchdog_timeout: 10s
  id: id_http

wifi:
  # Your WiFi configuration

# Transit component
transit_511:
  id: transit_id
  sources:
    - "https://api.511.org/transit/StopMonitoring?agency=SF&format=json&api_key=YOUR_API_KEY&stopcode=15201"
    - "https://api.511.org/transit/StopMonitoring?agency=AC&format=json&api_key=YOUR_API_KEY&stopcode=51234"
  refresh_interval: 5min
  max_response_buffer_size: 8k
```

### Advanced Configuration

```yaml
transit_511:
  id: transit_id
  sources:
    - "https://api.511.org/transit/StopMonitoring?agency=SF&format=json&api_key=YOUR_API_KEY&stopcode=15201"
  
  # Refresh settings
  refresh_interval: 2min
  max_response_buffer_size: 16k
  max_eta: 60min  # Only show ETAs within next hour
  
  # Route filtering (optional)
  route_filter:
    - N
    - 38
    - NOWL
  
  # Color customization
  default_route_color: !lambda return Color(0, 128, 0);  # Green
  separator_color: !lambda return Color(64, 64, 64);     # Dark gray
  
  route_colors:
    N: !lambda return Color(0, 100, 255);
    "38": !lambda return Color(255, 69, 0);
    NOWL: !lambda return Color(0, 0, 139);
  
  direction_colors:
    IB: !lambda return Color(245, 245, 245);  # Whitesmoke for Inbound
    OB: !lambda return Color(147, 112, 219);  # Medium purple for Outbound
```

## Configuration Options

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `id` | ID | Yes | - | Component identifier |
| `sources` | List | Yes | - | List of 511.org API URLs |
| `refresh_interval` | Time | No | 5min | How often to fetch new data |
| `max_response_buffer_size` | Size | No | 1kB | Maximum HTTP response buffer |
| `max_eta` | Time | No | 60min | Maximum ETA time to display |
| `route_filter` | List | No | - | Only show these route names |
| `default_route_color` | Color | No | - | Default color for routes |
| `separator_color` | Color | No | - | Color for separators |
| `route_colors` | Map | No | - | Custom colors per route |
| `direction_colors` | Map | No | - | Custom colors per direction |

## API Usage

### Getting Transit Data

```cpp
// Get all routes with ETAs
auto routes = id(transit_id).get_routes();
for (const auto& route : routes) {
    ESP_LOGD("transit", "Route: %s, ETAs: %d", 
             route.first.c_str(), route.second.size());
}

// Check if specific route is active (has upcoming ETAs)
if (id(transit_id).is_route_active("N")) {
    ESP_LOGD("transit", "N-Judah has upcoming trains");
}

// Get number of active routes
uint active_count = id(transit_id).get_num_active_routes();
```

### Manual Refresh

```cpp
// Force immediate refresh
id(transit_id).refresh(true);

// Check if currently refreshing
if (id(transit_id).running()) {
    ESP_LOGD("transit", "Currently fetching data...");
}
```

### Debug Information

```cpp
// Print detailed debug information
id(transit_id).debug_print();
```

## Route Filtering

The `route_filter` parameter allows you to show only specific transit routes:

```yaml
transit_511:
  route_filter:
    - N         
    - "38"
    - NOWL
```

**Notes:**

- Route names are case-sensitive and must match exactly
- If `route_filter` is omitted, all routes are displayed
- Numeric route names should be quoted in YAML
- Filtered routes are logged during operation for debugging

## Color Customization

### Route Colors

Assign specific colors to transit routes for easy identification:

```yaml
route_colors:
  N: COLOR_BLUE
  "38": COLOR_GREEN
```

### Direction Colors

Different colors for inbound vs outbound directions:

```yaml
direction_colors:
  IB: COLOR_WHITE        # Inbound in white
  OB: COLOR_PURPLE       # Outbound in purple
```

## 511.org API Setup

1. Register at [511.org](https://511.org/open-data/token) for a free API token
2. Use the StopMonitoring API endpoint:

   ```text
   https://api.511.org/transit/StopMonitoring?agency=AGENCY&format=json&api_key=YOUR_KEY&stopcode=STOP_CODE
   ```

3. Find stop codes using the 511.org website

## Data Structure

The component provides access to transit data through these structures:

```cpp
struct transitRouteETA {
    std::string reference;      // Stop reference ID
    std::string Name;           // Route name (e.g., "N", "38")
    std::string Direction;      // "IB" or "OB"
    time_t RecordedAtTime;      // When data was recorded
    time_t ETA;                 // Expected arrival timestamp
    time_t ResponseTimestamp;   // API response timestamp
    bool live;                  // True if real-time tracking
    bool rail;                  // True if rail service
    Color directionColor;       // Color for direction
    Color routeColor;          // Color for route
};
```

## Example Display Integration

```yaml
display:
  - platform: your_display_platform
    lambda: |-
      auto routes = id(transit_id).get_routes();
      int y = 0;
      
      for (const auto& route_pair : routes) {
        const auto& route_name = route_pair.first;
        const auto& etas = route_pair.second;
        
        // Display route name
        it.printf(0, y, id(font), id(transit_id).get_route_color(route_name),
                 "%s:", route_name.c_str());
        
        // Display ETAs
        for (size_t i = 0; i < std::min(etas.size(), 3UL); i++) {
          auto eta_minutes = (etas[i]->ETA - id(sntp_time).now().timestamp) / 60;
          it.printf(20 + i*15, y, id(font), 
                   etas[i]->directionColor, "%dm", eta_minutes);
        }
        y += 10;
      }
```

## Troubleshooting

### Common Issues

1. **No data received**: Check API key validity and network connectivity
2. **Old data**: Verify time component is working and timezone is correct  
3. **Missing routes**: Check route names in `route_filter` match exactly
4. **Memory errors**: Increase `max_response_buffer_size` if responses are large

### Debug Logging

Enable debug logging to troubleshoot issues:

```yaml
logger:
  level: DEBUG
  logs:
    transit_511: DEBUG
```

### API Rate Limits

511.org has rate limits (~1 request/minute). Space out refresh intervals accordingly:

```yaml
transit_511:
  refresh_interval: 90s  # Stay well under rate limit
```

## Example Render

![transit_511 screenshot](511_matrix.webp)

## License

This component is provided as-is for use with ESPHome projects.
