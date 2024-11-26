# Tetris Animation

Based on https://github.com/toblum/TetrisAnimation

Modified for ESPHome

![tetris clock](https://github.com/user-attachments/assets/871b076d-4c8c-47ad-b68b-1155ede96189)

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
