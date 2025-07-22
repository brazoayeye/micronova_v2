import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_STEP, DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS

from .. import *

CONF_ON_VALUE = "on_value"
CONF_OFF_VALUE = "off_value"

MicroNovaSwitch = micronova_ns.class_("MicroNovaSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MICRONOVA_ID): cv.use_id(MicroNova),
        cv.Optional(CONF_CUSTOM, default=[]): cv.ensure_list( 
        switch.switch_schema(MicroNovaSwitch)
            .extend(MICRONOVA_LOCATION_SCHEMA)
            .extend({
                cv.Optional(CONF_ON_VALUE, default=255): cv.float_,
                cv.Optional(CONF_OFF_VALUE, default=0): cv.float_,
                cv.Optional(CONF_RETAIN, default=False): cv.boolean
            })
        )
        
    }
)


async def to_code(config):
    #mv = await cg.get_variable(config[CONF_MICRONOVA_ID])
    for cfg in config[CONF_CUSTOM]:
        el = await switch.new_switch(cfg)
        await cg.register_component(el, cfg)

        cg.add(el.MicroNovaParameter_setAll( #uint16_t addr, uint16_t ee, uint8_t size, float priority, uint16_t mask
            cfg[CONF_MEMORY_ADDRESS], 
            cfg[CONF_MEMORY_EEPROM], 
            cfg[CONF_MEMORY_SIZE], 
            cfg[CONF_PRIORITY], 
            cfg[CONF_MEMORY_MASK],
            cfg[CONF_SLOPE],
            cfg[CONF_OFFSET]))
        cg.add(el.MicroNovaSwitch_setOnOff(cfg[CONF_ON_VALUE], cfg[CONF_OFF_VALUE], cfg[CONF_RETAIN]))
