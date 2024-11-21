CODEOWNERS = ["@lanrat"]

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, number
from esphome.const import (
    CONF_SENSOR_ID,
    CONF_ID,
)

CONF_NUMBER_ID = "number_id"
CONF_DECREASE_OFFSET = "decrease_offset"
CONF_LEVELS = "levels"

auto_brightness_ns = cg.esphome_ns.namespace("auto_brightness")

AutoBrightness = auto_brightness_ns.class_(
    "AutoBrightness", cg.Component
)

def validate_levels(value):
    if isinstance(value, list):
        return cv.Schema(
                cv.All(
                    cv.ensure_list(cv.float_),
                    cv.Length(min=2, max=2)
                    ),
        )(value)
    value = cv.string(value)
    if "->" not in value:
        raise cv.Invalid("Levels mapping must contain '->'")
    
    a, b = value.split("->", 1)
    a, b = a.strip(), b.strip()
    return validate_levels([cv.float_(a), cv.float_(b)])


CONFIG_SCHEMA = cv.Schema({
  cv.GenerateID(): cv.declare_id(AutoBrightness),
  cv.Required(CONF_SENSOR_ID): cv.use_id(sensor.Sensor),
  cv.Required(CONF_NUMBER_ID): cv.use_id(number.Number),
  cv.Required(CONF_LEVELS): cv.All(
    cv.ensure_list(validate_levels), cv.Length(min=2)
  ),
  cv.Optional(CONF_DECREASE_OFFSET, default=5.0): cv.float_,
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    sens = await cg.get_variable(config[CONF_SENSOR_ID])
    cg.add(var.set_sensor(sens))
    
    num = await cg.get_variable(config[CONF_NUMBER_ID])
    cg.add(var.set_number(num))

    cg.add(var.set_decrease_offset(config[CONF_DECREASE_OFFSET]))
    
    cg.add(var.set_levels(config[CONF_LEVELS]))
    

