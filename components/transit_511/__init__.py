CODEOWNERS = ["@lanrat"]
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.http_request import (
    CONF_HTTP_REQUEST_ID,
    CONF_MAX_RESPONSE_BUFFER_SIZE,
    HttpRequestComponent,
    )
from esphome.components import time, wifi
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
    CONF_WIFI,
)

DEPENDENCIES = ["time", "http_request", "wifi"]

CONF_SOURCES = "sources"
CONF_REFRESH_INTERVAL = "refresh_interval"

transit_511_ns = cg.esphome_ns.namespace("transit_511")

Transit511 = transit_511_ns.class_(
    "Transit511", cg.Component
)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(Transit511),
    cv.GenerateID(CONF_HTTP_REQUEST_ID): cv.use_id(HttpRequestComponent),
    cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
    cv.GenerateID(CONF_WIFI): cv.use_id(wifi.WiFiComponent),
    cv.Required(CONF_SOURCES): cv.All(
        cv.ensure_list(cv.string), cv.Length(min=1)
    ),
    cv.Optional(
        CONF_REFRESH_INTERVAL, default="5min"
    ): cv.positive_time_period_minutes,
    cv.Optional(CONF_MAX_RESPONSE_BUFFER_SIZE, default="1kB"): cv.validate_bytes,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
        
    time_ = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_))
    
    wifi_ = await cg.get_variable(config[CONF_WIFI])
    cg.add(var.set_wifi(wifi_))
    
    http_ = await cg.get_variable(config[CONF_HTTP_REQUEST_ID])
    cg.add(var.set_http(http_))
    
    cg.add(var.set_refresh(config[CONF_REFRESH_INTERVAL].total_milliseconds))
    cg.add(var.set_max_response_buffer_size(config[CONF_MAX_RESPONSE_BUFFER_SIZE]))
    
    sources = config[CONF_SOURCES]
    for source in sources:
        cg.add(var.add_source(source))
