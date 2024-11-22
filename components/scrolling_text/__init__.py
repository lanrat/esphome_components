CODEOWNERS = ["@lanrat"]

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
)

scrolling_text_ns = cg.esphome_ns.namespace("scrolling_text")

ScrollingText = scrolling_text_ns.class_(
    "ScrollingText", cg.Component
)

CONFIG_SCHEMA = cv.Schema({
  cv.GenerateID(): cv.declare_id(ScrollingText),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)