#include "micronova_v2.h"
#include "esphome/core/log.h"

namespace esphome {

namespace micronova_v2 {

std::vector<MicroNovaBoadLocation*> MicroNovaBoadLocation::list;
std::vector<MicroNova_v2::writeReq_t> MicroNova_v2::writeList;
void MicroNovaParameter::setValue(float v) {
  MicroNova_v2::write_BoardLocation(this, (v - offset) / slope + 0.2f);
}

void MicroNova_v2::setup() {
  if (this->enable_rx_pin_ != nullptr) {
    this->enable_rx_pin_->setup();
    this->enable_rx_pin_->pin_mode(gpio::FLAG_OUTPUT);
    this->enable_rx_pin_->digital_write(false);
  }
}

void MicroNova_v2::dump_config() {
  ESP_LOGCONFIG(TAG, "MicroNova:");
  if (this->enable_rx_pin_ != nullptr) LOG_PIN("  Enable RX Pin: ", this->enable_rx_pin_);
  ESP_LOGCONFIG(TAG, "Echo: ", this->uart_echo_);
  for (auto &bl : MicroNovaBoadLocation::list) {
    ESP_LOGCONFIG(TAG, "    location %c0x%X size=%u", bl->ee?'E':'R', bl->addr, bl->size);
  }
}

void MicroNova_v2::loop() {
  auto list = MicroNovaBoadLocation::list;
  static uint8_t wbuf[4];
  static uint8_t rbuf[4];
  switch (currentState){
    case currentState_idle:
      if (writeList.size() > 0) { // something to write
        writeReq_t req = writeList.front();
        currentIndex = req.par->location;
        MicroNovaBoadLocation * loc = list[currentIndex];
        if (loc->size == 1 && req.par->mask == 0xFF || loc->size == 2 && req.par->mask == 0xFFFF) {
          goState(currentState_writeSend);
        } else {
          goState(currentState_readSend); // must read before to apply mask
        }
        ESP_LOGI(TAG, "ToWrite%u: state%u %c0x%x = %u mod %u",loc->size, currentState, loc->ee ? 'E':'R', loc->addr, req.val, req.par->mask);
      } else { // selecting another
        uint32_t now = millis();
        for (uint8_t i = 0; i < list.size(); i++) {
          currentIndex = (currentIndex+1)%list.size();
          float dt = float(now - list[currentIndex]->lastRead)/1000;
          float limit = list[currentIndex]->priority * multiplier;
          if (list[currentIndex]->readAtLeastOnce) {
            if (dt/limit > 1)  multiplier *= 1.01;
            if (dt/limit < 0.9)  multiplier *= 0.99;
            if (multiplier > 5) {
              multiplier = 5;
              //ESP_LOGW(TAG, "Multipier is >5! Communication problems?");
            } else {
              //ESP_LOGD(TAG, "Multiplier: %f", multiplier);
            }
          }
          if (dt > limit) {
            goState(currentState_readSend);
            break;
          }
        }
        return;
      }
      break;
    case currentState_readSend:
    case currentState_readSend2:{
      while (this->available()) {
        this->read_byte(rbuf);
        ESP_LOGW(TAG, "Reading excess byte 0x%02X", rbuf[0]);
      }
      uint16_t addr = list[currentIndex]->addr + (currentState == currentState_readSend2);
      wbuf[0] = (list[currentIndex]->ee << 5) | (addr>> 8);
      wbuf[1] = addr & 0xff;
      //ESP_LOGV(TAG, "State %u [%02X,%02X]", currentState, wbuf[0], wbuf[1]);
      if (this->enable_rx_pin_ != nullptr) this->enable_rx_pin_->digital_write(true);
      this->write_array(wbuf, 2);
      this->flush();
      if (this->enable_rx_pin_ != nullptr) this->enable_rx_pin_->digital_write(false);
      goState((currentState_t)(currentState+1));
      return;}

    case currentState_writeSend:
    case currentState_writeSend2:{
      while (this->available()) {
        this->read_byte(rbuf);
        ESP_LOGW(TAG, "Reading excess byte 0x%02X", rbuf[0]);
      }
      uint16_t addr = list[currentIndex]->addr + (currentState == currentState_writeSend2);
      wbuf[0] = 0x80 | (list[currentIndex]->ee << 5) | (addr >> 8);
      wbuf[1] = addr & 0xff;
      writeReq_t req = writeList.front();
      MicroNovaParameter * par = req.par;
      uint16_t mask = par->mask;
      uint16_t val = (req.val & mask) | (list[currentIndex]->value & ~mask);
      ESP_LOGV(TAG, "!! %u & %u | %u & %u = %u", req.val, mask, list[currentIndex]->value,  ~mask, val);
      wbuf[2] =  currentState == currentState_writeSend2 ? val >> 8 : val & 0xFF;
      wbuf[3] = wbuf[2] + wbuf[1] + wbuf[0];

      ESP_LOGV(TAG, "State %u [%02X,%02X,%02X,%02X]", currentState, wbuf[0], wbuf[1], wbuf[2], wbuf[3]);
      if (this->enable_rx_pin_ != nullptr) this->enable_rx_pin_->digital_write(true);
      this->write_array(wbuf, 4);
      this->flush();
      if (this->enable_rx_pin_ != nullptr) this->enable_rx_pin_->digital_write(false);
      goState((currentState_t)(currentState+1));
      return;}

    case currentState_readWait:
    case currentState_readWait2:
    case currentState_writeWait:
    case currentState_writeWait2:{
      uint8_t echoRead = uart_echo_ ? 2 + 2 * (currentState == currentState_writeWait2 || currentState == currentState_writeWait) : 0;
      if (this->available() >= 2+echoRead) {
        this->read_array(rbuf, echoRead);
        this->read_array(rbuf, 2);
        uint8_t chk = rbuf[1] + wbuf[0] + wbuf[1];
        if (rbuf[0] !=chk) {
          ESP_LOGE(TAG, "Error during checksum calc:%X read:%X", chk, rbuf[0]);
          if (currentState == currentState_readWait || currentState == currentState_readWait2){
            ESP_LOGE(TAG, "Sent:%02X %02X read:%02X %02X", wbuf[0], wbuf[1], rbuf[0], rbuf[1]);
          } else {
            ESP_LOGE(TAG, "Sent:%02X %02X %02X %02X read:%02X %02X", wbuf[0], wbuf[1],wbuf[2], wbuf[3], rbuf[0], rbuf[1]);
          }
          goState(currentState_idle);
          }

        if (currentState == currentState_readWait || currentState == currentState_writeWait){
          list[currentIndex]->value = rbuf[1];
          if (list[currentIndex]->size == 1) { // byte 1 of 1
            list[currentIndex]->readAtLeastOnce = true;
            list[currentIndex]->lastRead = millis();
            //ESP_LOGV(TAG, "Value for location %u = %u", currentIndex, rbuf[1]);
            for (auto &cb : list[currentIndex]->callbacks) cb();
            if (currentState == currentState_readWait) {
              goState(isWritingRoutine() ? currentState_writeSend : currentState_idle);
            } else {
              goState(currentState_idle);
              writeList.erase(writeList.begin());
            }
          } else {   // byte 1 of 2
            goState((currentState_t) (currentState+1));
          }
        } else {  // byte 2 of 2
          list[currentIndex]->readAtLeastOnce = true;
          list[currentIndex]->lastRead = millis();
          list[currentIndex]->value += ((uint16_t)rbuf[1])<<8;
          //ESP_LOGV(TAG, "Value for location %u = %u", currentIndex, rbuf[1]);
          for (auto &cb : list[currentIndex]->callbacks) cb();
          if (currentState == currentState_readWait2) {
            goState(isWritingRoutine() ? currentState_writeSend : currentState_idle);
          } else {
              goState(currentState_idle);
              writeList.erase(writeList.begin());
          }
        }
      } else {
        if (millis()-stateTime > 200) {
          ESP_LOGE(TAG, "Timeout for parameter %c0x%X with state %u", list[currentIndex]->ee?'E':'R', list[currentIndex]->addr, currentState);
          goState(currentState_idle);
        }
        return;
      }
    }
  }
  loop();
}

}  // namespace micronova
}  // namespace esphome
