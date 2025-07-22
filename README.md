# Micronova_v2

This project provides an ESPHome component similar to the [official Micronova integration](https://github.com/brazoayeye/esphome/tree/dev/esphome/components/micronova), but with a different feature set.

The main goal of this component is to offer a flexible set of sensors, switches, and number controls that are not directly tied to specific stove functions, allowing users to adapt the YAML configuration to their particular device.

A secondary objective is to improve performance. For this reason, the component avoids using the standard PollingSensor class and instead analyzes the UART bus as quickly as possible to provide near real-time information.

For configuration details, see example.yaml.