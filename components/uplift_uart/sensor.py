import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import (
    CONF_ID,
    STATE_CLASS_MEASUREMENT,
)

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@lanrat"]

uplift_uart_ns = cg.esphome_ns.namespace("uplift_uart")
UpliftUartSensor = uplift_uart_ns.class_(
    "UpliftUartSensor", sensor.Sensor, cg.PollingComponent, uart.UARTDevice
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        UpliftUartSensor,
        unit_of_measurement="in",
        accuracy_decimals=1,
        state_class=STATE_CLASS_MEASUREMENT,
        icon="mdi:arrow-expand-vertical",
    )
    .extend(cv.polling_component_schema("100ms"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
