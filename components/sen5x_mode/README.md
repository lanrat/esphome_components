# SEN5x Measurement Mode

Runtime control of the measurement mode on a Sensirion [SEN5x](https://esphome.io/components/sensor/sen5x.html)
(SEN50/SEN54/SEN55), so the fan and laser can be duty-cycled instead of running continuously.

## Why

The stock `sen5x` component chooses a measurement mode once, in `setup()`, based on whether any
particulate sensors are declared, and never changes it. Declare `pm_2_5` and the fan and laser run
for the life of the device; leave it out and you never get particulate readings at all.

The SEN5x has a *gas-only* mode (`0x0037`) that keeps temperature, humidity, VOC index and NOx index
running with the fan and laser **off**. Parking the sensor there and only switching to full
measurement for a short particulate sample drops the duty cycle on the mechanical and optical parts
to a few percent.

Fan and laser are also most of the sensor's power draw — roughly 65mA of a SEN55's ~70mA — so this
matters as much for battery-powered nodes as it does for sensor lifetime.

## VOC/NOx algorithm state

Mode changes are only accepted while the sensor is idle, so the switch is `stop` → wait 200ms →
`start`. Stopping resets the VOC and NOx index algorithms, which at any useful sampling interval
would leave both permanently uncalibrated.

This component reads the algorithm state out (`0x6181`) before stopping and writes it back while
idle, so the index values survive the switch. This is independent of the stock component's
`store_baseline`, which only persists the same state to flash across reboots, and only every 2 hours.

## Particulate readings in gas-only mode

While in gas-only mode the particulate registers read back `0xFFFF`, which the stock component
publishes as `NaN`. Add `filter_out: nan` to each PM sensor so they hold their last real value
instead of going unavailable between samples.

## Configuration

Requires an empty top-level block. ESPHome only imports a component that the YAML names, and the
actions register on import.

```yaml
sen5x_mode:
```

## Actions

- `sen5x_mode.set_full` — fan and laser on; particulates, temperature, humidity, VOC, NOx.
- `sen5x_mode.set_gas_only` — fan and laser off; temperature, humidity, VOC, NOx only.

Both take the id of the `sen5x` sensor.

## Example

Sample particulates for 30s every 10 minutes, and leave the fan off the rest of the time.

```yaml
sen5x_mode:

sensor:
  - platform: sen5x
    id: sen55
    address: 0x69
    update_interval: 10s
    pm_2_5:
      name: "PM 2.5"
      filters:
        - filter_out: nan
    voc:
      name: "VOC"
    nox:
      name: "NOx"
    temperature:
      name: "Temperature"
    humidity:
      name: "Humidity"

script:
  - id: pm_sample
    mode: single
    then:
      - sen5x_mode.set_full: sen55
      - delay: 30s
      - sen5x_mode.set_gas_only: sen55

interval:
  - interval: 10min
    then:
      - script.execute: pm_sample

esphome:
  on_boot:
    - priority: -100
      then:
        - sen5x_mode.set_gas_only: sen55
```

Note the sensor still needs at least one PM sensor declared, so that the stock component starts in
full mode and the particulate values get published at all; the `on_boot` action moves it to gas-only
once running.
