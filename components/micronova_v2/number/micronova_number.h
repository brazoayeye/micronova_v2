#pragma once

#include "../micronova_v2.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace micronova_v2 {

class MicroNovaNumber : public Component, public number::Number, public MicroNovaParameter{
 public:
  MicroNovaNumber() {}

  void setup() override {
    ESP_LOGI(TAG, "Registering callback for %s", this->get_name().c_str());
    MicroNovaBoadLocation::list[this->location]->callbacks.push_back([this]() {
      uint16_t v = MicroNovaBoadLocation::list[this->location]->value & this->mask;
      if (this->lastSent != v){
        ESP_LOGV(TAG, "%s: %u => %f, last=%u", this->get_name().c_str(), v, getValue(), this->lastSent);
        this->lastSent = v;
        this->publish_state(getValue());
        }
      });
    ESP_LOGI(TAG, "Now there are %u callbacks fot that address", MicroNovaBoadLocation::list[this->location]->callbacks.size());
  }
  void dump_config() override { LOG_NUMBER("", "Micronova number", this); }
  void control(float value) override {
    ESP_LOGD(TAG, "New value to WRITE for %s @ %f", this->get_name().c_str(), value);
    this->setValue(value);
  };

 protected:
};

}  // namespace micronova
}  // namespace esphome
