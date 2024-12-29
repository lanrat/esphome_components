import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID

from .. import AUTO_BRIGHTNESS_ID, AutoBrightness

auto_brightness_switch_ns = cg.esphome_ns.namespace("auto_brightness::auto_brightness_switch")

AutoBrightnessSwitch = auto_brightness_switch_ns.class_(
    "AutoBrightnessSwitch", switch.Switch, cg.Component
)

CONFIG_SCHEMA = switch.SWITCH_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(AutoBrightnessSwitch),
        cv.Required(AUTO_BRIGHTNESS_ID): cv.use_id(AutoBrightness),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    auto_brightness = await cg.get_variable(config[AUTO_BRIGHTNESS_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    cg.add(auto_brightness.register_enable_switch(var))
