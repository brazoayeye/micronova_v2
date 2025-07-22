from esphome import pins
import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@brazoayeye"]

DEPENDENCIES = ["uart"]

CONF_MICRONOVA_ID = "micronovav2_id"
CONF_ENABLE_RX_PIN = "enable_rx_pin"
CONF_UART_ECHO = "uart_echo"
CONF_MEMORY_EEPROM = "memory_eeprom"
CONF_MEMORY_SIZE = "memory_size"
CONF_MEMORY_ADDRESS = "memory_address"
CONF_MEMORY_MASK = "memory_mask"
CONF_PRIORITY = "priority"
CONF_SLOPE = "slope"
CONF_OFFSET = "offset"
CONF_CUSTOM = "custom"
CONF_RETAIN = "retain"

micronova_ns = cg.esphome_ns.namespace("micronova_v2")

MicroNova = micronova_ns.class_("MicroNova_v2", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MicroNova),
            cv.Optional(CONF_ENABLE_RX_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_UART_ECHO, default=False): cv.boolean,
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
)


MICRONOVA_LOCATION_SCHEMA = cv.Schema(
        {
            cv.GenerateID(CONF_MICRONOVA_ID): cv.use_id(MicroNova),
            cv.Required(CONF_MEMORY_ADDRESS): cv.int_range(min=0x00, max=0x1FFF),
            cv.Optional(CONF_MEMORY_EEPROM, default=False): cv.boolean,
            cv.Optional(CONF_MEMORY_SIZE, default=1): cv.int_range(min=1, max=2),
            cv.Optional(CONF_MEMORY_MASK, default=0xFF): cv.int_range(min=0x01, max=0xFFFF),
            cv.Optional(CONF_PRIORITY, default=1): cv.float_,
            cv.Optional(CONF_SLOPE, default=1): cv.float_,
            cv.Optional(CONF_OFFSET, default=0): cv.float_,
        }
    )


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    if CONF_ENABLE_RX_PIN in config:
        enable_rx_pin = await cg.gpio_pin_expression(config[CONF_ENABLE_RX_PIN])
        cg.add(var.set_enable_rx_pin(enable_rx_pin))
    cg.add(var.set_uart_echo(config[CONF_UART_ECHO]))
