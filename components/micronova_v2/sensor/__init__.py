import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import CONF_STEP, DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS

from .. import *

MicroNovaSensor = micronova_ns.class_("MicroNovaSensor", sensor.Sensor, cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_MICRONOVA_ID): cv.use_id(MicroNova),
        cv.Optional(CONF_CUSTOM, default=[]): cv.ensure_list( 
        sensor.sensor_schema(MicroNovaSensor)
            .extend(MICRONOVA_LOCATION_SCHEMA)
        )
        
    }
)


async def to_code(config):
    #mv = await cg.get_variable(config[CONF_MICRONOVA_ID])
    for cfg in config[CONF_CUSTOM]:
        el = await sensor.new_sensor(cfg)
        await cg.register_component(el, cfg)

        cg.add(el.MicroNovaParameter_setAll( #uint16_t addr, uint16_t ee, uint8_t size, float priority, uint16_t mask, slope, offset
            cfg[CONF_MEMORY_ADDRESS], 
            cfg[CONF_MEMORY_EEPROM], 
            cfg[CONF_MEMORY_SIZE], 
            cfg[CONF_PRIORITY], 
            cfg[CONF_MEMORY_MASK],
            cfg[CONF_SLOPE],
            cfg[CONF_OFFSET]))
