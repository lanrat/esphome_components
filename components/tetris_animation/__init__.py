CODEOWNERS = ["@lanrat"]

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import time
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
)

CONF_SCALE = "scale"

tetris_animation_ns = cg.esphome_ns.namespace("tetris_animation")

TetrisAnimation = tetris_animation_ns.class_(
    "TetrisAnimation", cg.Component
)

CONFIG_SCHEMA = cv.Schema({
  cv.GenerateID(): cv.declare_id(TetrisAnimation),
  cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
  cv.Optional(CONF_SCALE, default=2): cv.int_range(min=1),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    time_source = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time_source(time_source))

    cg.add(var.set_scale(config[CONF_SCALE]))

