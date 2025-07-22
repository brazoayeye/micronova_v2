#pragma once

#include "../micronova_v2.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace micronova_v2 {

class MicroNovaSwitch : public Component, public switch_::Switch, public MicroNovaParameter{
 public:
  MicroNovaSwitch() {}

  void setup() override {
    ESP_LOGI(TAG, "Registering callback for %s", this->get_name().c_str());
    MicroNovaBoadLocation::list[this->location]->callbacks.push_back([this]() {
      bool v = (MicroNovaBoadLocation::list[this->location]->value & this->mask) > 0;
      if (this->lastSent != v){
        ESP_LOGV(TAG, "%s: %u => %f, last=%u", this->get_name().c_str(), v, getValue(), this->lastSent);
        this->lastSent = v;
        this->publish_state(v);
        }
      if (retain && v != lastRetained) {
        ESP_LOGV(TAG, "Retain: %u (%u mod %u>0) differs from retained %u", 
          v, MicroNovaBoadLocation::list[this->location]->value, this->mask, lastRetained);
        if (MicroNova_v2::writeList.size() == 0) write_state(lastRetained); //can't send multiple values if they're bits of the same byte
        }
      });
    ESP_LOGI(TAG, "Now there are %u callbacks fot that address", MicroNovaBoadLocation::list[this->location]->callbacks.size());
  }
  void dump_config() override { LOG_SWITCH("", "Micronova switch", this); }

  void MicroNovaSwitch_setOnOff(uint16_t on, uint16_t off, bool retainn){ on_value = on; off_value = off; retain = retainn;}

 protected:
  uint16_t on_value, off_value;
  bool retain, lastRetained = false;
  void write_state(bool state) override {
    lastRetained = state;
    uint16_t new_val = state ? this->mask & on_value : this->mask & off_value;
    ESP_LOGD(TAG, "Writing %s to %s (%u)", state ? "ON" : "OFF", this->get_name().c_str(), new_val);
    MicroNova_v2::write_BoardLocation(this, new_val);
  }
};

}  // namespace micronova
}  // namespace esphome
