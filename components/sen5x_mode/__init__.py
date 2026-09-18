import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components.sen5x.sensor import SEN5XComponent
from esphome.const import CONF_ID

CODEOWNERS = ["@lanrat"]

sen5x_mode_ns = cg.esphome_ns.namespace("sen5x_mode")

SetModeAction = sen5x_mode_ns.class_("SetModeAction", automation.Action)
MeasurementMode = sen5x_mode_ns.enum("MeasurementMode")

# This component only contributes actions. It still needs an empty top-level
# `sen5x_mode:` block in the config, because the action registry is populated on
# module import and ESPHome only imports components named in the YAML.
CONFIG_SCHEMA = cv.Schema({})

ACTION_SCHEMA = automation.maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(SEN5XComponent),
    }
)


async def to_code(config):
    pass


async def _mode_action_to_code(config, action_id, template_arg, args, mode):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    cg.add(var.set_mode(mode))
    return var


@automation.register_action(
    "sen5x_mode.set_full", SetModeAction, ACTION_SCHEMA, synchronous=True
)
async def sen5x_mode_set_full_to_code(config, action_id, template_arg, args):
    return await _mode_action_to_code(
        config, action_id, template_arg, args, MeasurementMode.MODE_FULL
    )


@automation.register_action(
    "sen5x_mode.set_gas_only", SetModeAction, ACTION_SCHEMA, synchronous=True
)
async def sen5x_mode_set_gas_only_to_code(config, action_id, template_arg, args):
    return await _mode_action_to_code(
        config, action_id, template_arg, args, MeasurementMode.MODE_GAS_ONLY
    )
