# Scrolling Text

Scroll Text on an ESPHome Display

```yaml
scrolling_text:
  id: scrolling_text_id

display:
    - id: display
      lambda: id(scrolling_text_id).draw(it, 0, 0);

script:
  - id: id_notification_script
    mode: restart
    parameters:
      message: string
      duration_sec: int
    then:
      - lambda: |-
          // default color
          auto color = id(my_color);
          // default delay between frames
          int delay_ms = 50;
          // center text which is small enough to not scroll
          bool center = true;
          
          // call printf
          id(scrolling_text_id).printf(id(matrix).get_width(), id(matrix).get_height(),
            id(my_font), color,
          delay_ms, center,
            "%s", message.c_str());

      - delay: !lambda |-
          // det default
          const auto default_delay_s = 30;
          auto delay = (duration_sec > 0) ? duration_sec : default_delay_s;
          return 1000 * delay; // sec to ms
      - wait_until: # wait till done scrolling at least once if long message
          timeout: 5min
          condition: 
              lambda: return id(scrolling_text_id).stop_when_clear();
```
