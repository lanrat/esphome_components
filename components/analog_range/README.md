# Analog Range Binary Sensor

This component exposes a [binary sensor](https://esphome.io/components/binary_sensor/) for when an analog sensor or [ADC](https://esphome.io/components/sensor/adc.html) is within a provided range.

## Hardware

There are cheap "keypads" sold online that adjust a resistive load differently based on which button is pressed. This component can expose individual buttons as binary sensors.

* [AD Keyboard](https://www.aliexpress.us/item/3256802254533505.html)
* [Analog Keyboard](https://www.aliexpress.us/item/3256801710797349.html)

### Photos

<img src="https://github.com/user-attachments/assets/dce93d90-4ab1-4ba3-8147-4519798efe05" width="30%"/>

<img src="https://github.com/user-attachments/assets/47534383-83b6-48cd-a824-de2335f4ed94" width="30%"/>

<img src="https://github.com/user-attachments/assets/e25bf375-adcc-4063-af75-c0586cc60636" width="30%"/>


## Example

```yaml
sensor:
  - platform: adc
    pin: 34
    attenuation: 12db
    name: "button_adc"
    internal: true
    id: id_button_adc
    update_interval: 10ms # WARNING: this prints a lot of logs

binary_sensor:
  - platform: analog_range
    icon: mdi:radiobox-marked
    sensor_id: id_button_adc
    name: "Blue Button"
    range:
        lower: 0.10
        upper: 0.20
    filters:
      - delayed_on_off: 20ms

  - platform: analog_range
    icon: mdi:radiobox-marked
    sensor_id: id_button_adc
    name: "Yellow Button"
    range:
        lower: 1.65
        upper: 1.69
    filters:
      - delayed_on_off: 20ms

  - platform: analog_range
    icon: mdi:radiobox-marked
    sensor_id: id_button_adc
    name: "Red Button"
    range:
        lower: 2.18
        upper: 2.25
    filters:
      - delayed_on_off: 20ms

  - platform: analog_range
    icon: mdi:radiobox-marked
    sensor_id: id_button_adc
    name: "Green Button"
    range:
        lower: 2.48
        upper: 2.52

    filters:
      - delayed_on_off: 20ms

  - platform: analog_range
    icon: mdi:radiobox-marked
    sensor_id: id_button_adc
    name: "Grey Button"
    range:
        lower: 2.63
        upper: 2.68
    filters:
      - delayed_on_off: 20ms

```
