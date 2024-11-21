CODEOWNERS = ["@lanrat"]

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import display, color
from esphome.const import (
    CONF_ID,
    CONF_DISPLAY_ID,
    CONF_TIME_ID,
)

CONF_STARTING_DENSITY = "starting_density"
CONF_COLOR_OFF = "color_off"
CONF_COLOR_AGE_1 = "color_age_1"
CONF_COLOR_AGE_2 = "color_age_2"
CONF_COLOR_AGE_n = "color_age_n"

game_of_life_ns = cg.esphome_ns.namespace("game_of_life")

GameOFLife = game_of_life_ns.class_(
    "GameOFLife", cg.Component
)

CONFIG_SCHEMA = cv.Schema({
  cv.GenerateID(): cv.declare_id(GameOFLife),
  cv.Required(CONF_DISPLAY_ID): cv.use_id(display.Display),
  cv.Optional(CONF_STARTING_DENSITY, default=20): cv.int_range(min=10),
  cv.Optional(CONF_COLOR_OFF): cv.use_id(color.ColorStruct),
  cv.Optional(CONF_COLOR_AGE_1): cv.use_id(color.ColorStruct),
  cv.Optional(CONF_COLOR_AGE_2): cv.use_id(color.ColorStruct),
  cv.Optional(CONF_COLOR_AGE_n): cv.use_id(color.ColorStruct),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    disp = await cg.get_variable(config[CONF_DISPLAY_ID])
    cg.add(var.set_display(disp))
    
    density = await cg.get_variable(config[CONF_STARTING_DENSITY])
    cg.add(var.set_starting_density(density))
    
    if color_off_config := config.get(CONF_COLOR_OFF):
        color_off = await cg.get_variable(color_off_config)
        cg.add(var.set_color_off(color_off))
    
    if color_age_1_config := config.get(CONF_COLOR_AGE_1):
        color_age_1 = await cg.get_variable(color_age_1_config)
        cg.add(var.set_color_age_1(color_age_1))

    if color_age_2_config := config.get(CONF_COLOR_AGE_2):
        color_age_2 = await cg.get_variable(color_age_2_config)
        cg.add(var.set_color_age_2(color_age_2))
        
    if color_age_n_config := config.get(CONF_COLOR_AGE_n):
        color_age_n = await cg.get_variable(color_age_n_config)
        cg.add(var.set_color_age_n(color_age_n))