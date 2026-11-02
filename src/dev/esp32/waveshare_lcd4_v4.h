/* MIT License - Copyright (c) 2019-2024 Francis Van Roie
   For full license information read the LICENSE file in the project folder */

#ifndef HASP_DEVICE_WAVESHARE_LCD4_V4_H
#define HASP_DEVICE_WAVESHARE_LCD4_V4_H

#if defined(WAVESHARE_LCD4_V4)

#include "esp32.h"

// CH32 PWM is inverted: the factory firmware (BSP v3.0.0 bsp_display_backlight_on) writes 0 for full brightness
#ifndef WS_LCD4_BACKLIGHT_INVERT
#define WS_LCD4_BACKLIGHT_INVERT 1
#endif

namespace dev {

// Waveshare ESP32-S3-Touch-LCD-4 hardware V4.0
// LCD reset, touch reset, power hold and backlight PWM live on a CH32V003 helper MCU (I2C 0x24),
// not on ESP32 GPIOs, so the expander must be initialized before the TFT and touch drivers start.
class WaveshareLcd4V4 : public Esp32Device {
  public:
    void init() override;
    void show_info() override;

    void set_backlight_pin(uint8_t pin) override;
    void set_backlight_invert(bool invert) override;
    bool get_backlight_invert() override;
    void set_backlight_level(uint8_t level) override;
    uint8_t get_backlight_level() override;
    void set_backlight_power(bool power) override;
    bool get_backlight_power() override;

  private:
    uint8_t _backlight_level  = 255;
    bool _backlight_power     = true;
    bool _backlight_invert    = WS_LCD4_BACKLIGHT_INVERT;
    bool _expander_ok         = false;
    bool _pwm_touched         = false;
    uint8_t _init_attempts    = 0;
    unsigned long _init_start = 0;
    unsigned long _init_ms    = 0;

    void update_backlight();
};

} // namespace dev

using dev::WaveshareLcd4V4;
extern dev::WaveshareLcd4V4 haspDevice;

#endif
#endif
