# Auto Brightness

Automatically adjusts a `esphome::Number` exposed from a screen with a brightness control, based on a mapping of values from an input sensor.

## Example

```yaml
# screen brightness number
number:
  - platform: ...
    id: screen_brightness
    name: "Brightness"

# light sensor
sensor:  
  - platform: ...
    id: light_sensor
    name: "Illuminance"
    update_interval: 1s
    filters:
      - throttle_average: 10s

auto_brightness:
  id: auto_brightness_id
  sensor_id: light_sensor
  number_id: screen_brightness
  levels:
    # map from max sensor reading -> brightness level
    - 6 -> 8
    - 15 -> 64
    - 50 -> 96
    - 150 -> 128
    - 500 -> 192

switch:
  - platform: auto_brightness
    auto_brightness_id: auto_brightness_id
    name: "Auto Brightness"
    id: auto_brightness_switch
    restore_mode: ALWAYS_ON
    entity_category: "diagnostic"
```
