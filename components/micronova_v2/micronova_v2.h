#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <vector>

namespace esphome {
namespace micronova_v2 {

static const char *const TAG = "micronova_v2";

class MicroNova_v2;

//////////////////////////////////////////////////////////////////////
class MicroNovaBoadLocation {
 public:
  // never use std initializer, but use add_if_needed
  MicroNovaBoadLocation(uint16_t addr, uint16_t ee, uint8_t size, float priority): 
    addr(addr), ee(ee), size(size), priority(priority){
      list.push_back(this);
    }

  uint16_t addr;
  uint16_t ee;
  uint8_t size;
  float priority;
  uint16_t value = 0;
  std::vector<std::function<void()>> callbacks;
  uint32_t lastRead = 0;
  bool readAtLeastOnce = false;

  static std::vector<MicroNovaBoadLocation*> list;

  static uint8_t add_if_needed(uint16_t addr, uint16_t ee, uint8_t size, float priority){
    for (uint8_t i = 0; i < list.size(); i++){
      auto &bl = list[i];
      if (addr == bl->addr && ee == bl->ee) {
        if (size != bl->size) ESP_LOGE(TAG, "Size differs in two parameters with same address %c0x%x",ee ? 'E':'R', addr);
        if (priority != bl->priority) {
          ESP_LOGW(TAG, "Different prio in different parameters with same addr %c0x%x",ee ? 'E':'R', addr);
          if (priority > bl->priority) bl->priority = priority;
        }
        return i;
      }
    }
    new MicroNovaBoadLocation(addr, ee, size, priority);
    return list.size()-1;
  }

 protected:
};

//////////////////////////////////////////////////////////////////////
class MicroNovaParameter {
 public:
  MicroNovaParameter() {}

  void MicroNovaParameter_setAll(uint16_t addr, uint16_t ee, uint8_t size, float priority, uint16_t maskk, float slopee, float offsett){
      mask = maskk;
      slope = slopee;
      offset = offsett;
      location = MicroNovaBoadLocation::add_if_needed(addr, ee, size, priority);
    }

  uint16_t mask;
  uint8_t location; // index in MicroNovaBoadLocation::list
  uint16_t lastSent;
  float slope, offset;

  float getValue() {return (MicroNovaBoadLocation::list[location]->value & mask) * slope + offset;}
  void setValue(float v);


 protected:
};

/////////////////////////////////////////////////////////////////////
// Main component class
class MicroNova_v2 : public Component, public uart::UARTDevice {
 public:
  MicroNova_v2() {}

  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_enable_rx_pin(GPIOPin *enable_rx_pin) { this->enable_rx_pin_ = enable_rx_pin; }
  void set_uart_echo(bool x) { this->uart_echo_ = x;}

  static void write_BoardLocation(MicroNovaParameter * par, uint16_t val) {
    ESP_LOGV(TAG, "TO WRITE VAL:%u", val);
    writeList.push_back(writeReq_t{.par=par, .val=val});
  };

  typedef struct {
    MicroNovaParameter * par;
    uint16_t val;
  } writeReq_t;
  static std::vector<writeReq_t> writeList;

 protected:
  GPIOPin *enable_rx_pin_{nullptr};
  bool uart_echo_{false};

  typedef enum {
    currentState_idle,
    currentState_readSend,
    currentState_readWait,
    currentState_readSend2,
    currentState_readWait2,
    currentState_writeSend,
    currentState_writeWait,
    currentState_writeSend2,
    currentState_writeWait2
  } currentState_t;
  currentState_t currentState = currentState_idle;
  uint32_t stateTime = 0;
  uint8_t currentIndex = 0;
  void goState(currentState_t s) { stateTime = millis(); currentState=s;}
  float multiplier = 1.0;


  bool isWritingRoutine() {
    if (writeList.size() > 0) {
      if (writeList.front().par->location == currentIndex) return true;
    }
    return false;
  }

};

}  // namespace micronova
}  // namespace esphome
