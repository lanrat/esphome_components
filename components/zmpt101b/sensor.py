import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor , voltage_sampler
from esphome.const import (
    CONF_SENSOR,
    CONF_ID,
    CONF_PIN,
    UNIT_VOLT,
    CONF_CALIBRATION,
    CONF_FREQUENCY,
    STATE_CLASS_MEASUREMENT,
    DEVICE_CLASS_VOLTAGE)

CONF_REFERENCE_VOLTAGE = 'reference_voltage'
CODEOWNERS = ['@lanrat']
CONF_NUMBER_OF_SAMPLES = 'num_of_samples'

AUTO_LOAD = ["voltage_sampler"]

FREQUENCY_OPTIONS = {
    '50hz': 50,
    '60hz': 60,
}

zmpt101b_ns = cg.esphome_ns.namespace('zmpt101b')
ZMPT101BSensor = zmpt101b_ns.class_('ZMPT101BSensor', sensor.Sensor, cg.PollingComponent)


CONFIG_SCHEMA =( sensor.sensor_schema(
    ZMPT101BSensor,
    unit_of_measurement=UNIT_VOLT,
    accuracy_decimals=2,
    device_class=DEVICE_CLASS_VOLTAGE,
    state_class=STATE_CLASS_MEASUREMENT
    ).extend({
        cv.Required(CONF_SENSOR): cv.use_id(voltage_sampler.VoltageSampler),
        cv.Optional(CONF_CALIBRATION, default=1): cv.float_,
        cv.Optional(CONF_REFERENCE_VOLTAGE, default=5.0): cv.float_,
        cv.Optional(CONF_NUMBER_OF_SAMPLES, default='20'): cv.int_,
        cv.Optional(CONF_FREQUENCY, default='60hz'): cv.enum(FREQUENCY_OPTIONS),
}).extend(cv.polling_component_schema('60s'))
)

async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)

    sens = await cg.get_variable(config[CONF_SENSOR])
    cg.add(var.set_source(sens))
    cg.add(var.set_conf_calibration(config[CONF_CALIBRATION]))
    cg.add(var.set_conf_number_of_samples(config[CONF_NUMBER_OF_SAMPLES]))
    cg.add(var.set_conf_frequency(config[CONF_FREQUENCY]))
    cg.add(var.set_conf_reference(config[CONF_REFERENCE_VOLTAGE]))
