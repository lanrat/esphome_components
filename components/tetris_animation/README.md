# Tetris Animation

Based on https://github.com/toblum/TetrisAnimation

Modified for ESPHome

Currently in development

## Example

```yaml
tetris_animation:
  id: tetris
  time_id: sntp_time

button:
  - platform: template
    name: "Reset Tetris"
    on_press:
      - lambda: id(tetris).reset();

display:
  - id: my_display
    lambda: id(tetris).draw(it)
```
