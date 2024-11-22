import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ID

from .. import GAME_OF_LIFE_ID, GameOfLife, SPEED_MAX, SPEED_MIN

game_of_life_number_ns = cg.esphome_ns.namespace("game_of_life::game_of_life_number")

GameOfLifeNumber = game_of_life_number_ns.class_(
    "GameOfLifeNumber", cg.Component
)

CONFIG_SCHEMA = number.NUMBER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(GameOfLifeNumber),
        cv.Required(GAME_OF_LIFE_ID): cv.use_id(GameOfLife),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    game_of_life = await cg.get_variable(config[GAME_OF_LIFE_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await number.register_number(var, config, min_value=SPEED_MIN, max_value=SPEED_MAX, step=1)

    cg.add(game_of_life.register_speed_number(var))
