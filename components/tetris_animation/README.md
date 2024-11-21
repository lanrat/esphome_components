# Tetris Animation

Based on https://github.com/toblum/TetrisAnimation

Modified for ESPHome

Currently in development

## Example

```yaml
tetris_animation:
  id: tetris
  display_id: my_display
  time_id: sntp_time

button:
  - platform: template
    name: "Reset Tetris"
    on_press:
      - lambda: id(tetris).reset();
```
