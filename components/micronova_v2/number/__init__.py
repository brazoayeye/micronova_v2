import esphome.codegen as cg
from esphome.components import number
import esphome.config_validation as cv
from esphome.const import CONF_STEP, DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS

from .. import *

MicroNovaNumber = micronova_ns.class_("MicroNovaNumber", number.Number, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MICRONOVA_ID): cv.use_id(MicroNova),
        cv.Optional(CONF_CUSTOM, default=[]): cv.ensure_list( 
        number.number_schema(MicroNovaNumber)
            .extend(MICRONOVA_LOCATION_SCHEMA)
            .extend({
                cv.Optional("min_value", default=0): cv.float_,
                cv.Optional("max_value", default=255): cv.float_,
                cv.Optional("step", default=1): cv.float_,
            })
        )
        
    }
)


async def to_code(config):
    #mv = await cg.get_variable(config[CONF_MICRONOVA_ID])
    for cfg in config[CONF_CUSTOM]:
        min_val = cfg.get("min_value", 0) 
        max_val = cfg.get("max_value", 255)
        step    = cfg.get("step", 1)

        el = await number.new_number(cfg, min_value=min_val, max_value=max_val, step=step)
        await cg.register_component(el, cfg)

        cg.add(el.MicroNovaParameter_setAll( #uint16_t addr, uint16_t ee, uint8_t size, float priority, uint16_t mask
            cfg[CONF_MEMORY_ADDRESS], 
            cfg[CONF_MEMORY_EEPROM], 
            cfg[CONF_MEMORY_SIZE], 
            cfg[CONF_PRIORITY], 
            cfg[CONF_MEMORY_MASK],
            cfg[CONF_SLOPE],
            cfg[CONF_OFFSET]))
