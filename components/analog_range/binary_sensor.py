import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, sensor
from esphome.const import (
    CONF_SENSOR_ID,
    CONF_RANGE,
)

analog_range_ns = cg.esphome_ns.namespace("analog_range")

AnalogRangeBinarySensor = analog_range_ns.class_(
    "AnalogRangeBinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONF_UPPER = "upper"
CONF_LOWER = "lower"

CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(AnalogRangeBinarySensor)
    .extend(
        {
            cv.Required(CONF_SENSOR_ID): cv.use_id(sensor.Sensor),
            cv.Required(CONF_RANGE): cv.Any(
                cv.float_,
                cv.Schema(
                    {
                        cv.Required(CONF_UPPER): cv.float_,
                        cv.Required(CONF_LOWER): cv.float_,
                    }
                ),
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)

    sens = await cg.get_variable(config[CONF_SENSOR_ID])
    cg.add(var.set_sensor(sens))

    if isinstance(config[CONF_RANGE], float):
        cg.add(var.set_upper_range(config[CONF_RANGE]))
        cg.add(var.set_lower_range(config[CONF_RANGE]))
    else:
        cg.add(var.set_upper_range(config[CONF_RANGE][CONF_UPPER]))
        cg.add(var.set_lower_range(config[CONF_RANGE][CONF_LOWER]))