# Conway's Game of Life Component

Play Conway's Game of Life on an [ESPHome Display!](https://esphome.io/components/display/index.html)

![game of life screenshot](https://github.com/user-attachments/assets/299a801c-80cd-4de4-8dfe-b45e15499566)

## Credits

Inspired from: https://github.com/jackmachiela/PhotoLife

```yaml
game_of_life:
  id: gol
  rows: 32
  cols: 64
  color_age_1: COLOR_CSS_GREEN
  color_age_2: COLOR_CSS_INDIANRED
  color_age_n: COLOR_CSS_DARKORANGE
  starting_density: 40
  spark: true

number:
  - platform: game_of_life
    id: game_of_life_speed
    name: "Game Of Life Speed"
    game_of_life_id: gol

display:
    - id: display
      lambda: id(gol).draw(it, 0, 0);
```
