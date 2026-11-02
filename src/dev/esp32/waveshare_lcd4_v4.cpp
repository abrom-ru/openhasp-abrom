/* MIT License - Copyright (c) 2019-2024 Francis Van Roie
   For full license information read the LICENSE file in the project folder */

#include "waveshare_lcd4_v4.h"

#if defined(WAVESHARE_LCD4_V4)

#include <Wire.h>
#include "hasp_conf.h"
#include "hasp_debug.h"

// CH32V003 register map, from Waveshare WS_CH32_IO library
#define CH32_I2C_ADDRESS 0x24
#define CH32_REG_DIRECTION 0x02
#define CH32_REG_OUTPUT 0x03
#define CH32_REG_PWM 0x05
#define CH32_PWM_MAX 247 // safe max used by ESPHome for the Waveshare CH32 boards

#define CH32_PIN_TOUCH_RST (1 << 1)
#define CH32_PIN_LCD_RST (1 << 3)
#define CH32_PIN_SYS_EN (1 << 5) // battery power-hold latch, must stay high
#define CH32_PIN_BEE_EN (1 << 6) // buzzer, keep low
#define CH32_OUTPUTS (CH32_PIN_SYS_EN | CH32_PIN_BEE_EN | CH32_PIN_LCD_RST | CH32_PIN_TOUCH_RST)

namespace dev {

// The CH32 does not reset together with the ESP32, so a reset in the middle of
// an I2C transaction can leave SDA held low. Clock it out before using the bus.
static void ch32_recover_bus()
{
    pinMode(TOUCH_SDA, INPUT_PULLUP);
    pinMode(TOUCH_SCL, OUTPUT_OPEN_DRAIN);
    digitalWrite(TOUCH_SCL, HIGH);
    delayMicroseconds(10);

    for(int i = 0; i < 9 && digitalRead(TOUCH_SDA) == LOW; ++i) {
        digitalWrite(TOUCH_SCL, LOW);
        delayMicroseconds(10);
        digitalWrite(TOUCH_SCL, HIGH);
        delayMicroseconds(10);
    }

    // STOP condition
    pinMode(TOUCH_SDA, OUTPUT_OPEN_DRAIN);
    digitalWrite(TOUCH_SDA, LOW);
    delayMicroseconds(10);
    digitalWrite(TOUCH_SCL, HIGH);
    delayMicroseconds(10);
    digitalWrite(TOUCH_SDA, HIGH);
    delayMicroseconds(10);

    pinMode(TOUCH_SDA, INPUT_PULLUP);
    pinMode(TOUCH_SCL, INPUT_PULLUP);
    delay(2);
}

static uint8_t ch32_last_error = 0;

static bool ch32_write(uint8_t reg, uint8_t value)
{
    for(uint8_t attempt = 0; attempt < 5; ++attempt) {
        Wire.beginTransmission(CH32_I2C_ADDRESS);
        Wire.write(reg);
        Wire.write(value);
        ch32_last_error = Wire.endTransmission();
        if(ch32_last_error == 0) return true;
        delay(10 + attempt * 10);
    }
    return false;
}

static int ch32_read(uint8_t reg)
{
    Wire.beginTransmission(CH32_I2C_ADDRESS);
    Wire.write(reg);
    if(Wire.endTransmission(false) != 0) return -1;
    if(Wire.requestFrom(CH32_I2C_ADDRESS, 1) != 1) return -1;
    return Wire.read();
}

void WaveshareLcd4V4::init()
{
    _init_start = millis();
    ch32_recover_bus();
    Wire.begin(TOUCH_SDA, TOUCH_SCL, (uint32_t)I2C_TOUCH_FREQUENCY);
    delay(20);

    // Same sequence as the factory firmware (Waveshare BSP v3.0.0 bsp_display_new):
    // only SYS_EN, BEE, LCD_RST and TP_RST are outputs; pull BEE and both resets low, then release
    for(_init_attempts = 1; _init_attempts <= 3 && !_expander_ok; ++_init_attempts) {
        if(!ch32_write(CH32_REG_DIRECTION, CH32_OUTPUTS) || !ch32_write(CH32_REG_OUTPUT, CH32_PIN_SYS_EN)) {
            delay(20);
            continue;
        }
        delay(200);
        if(!ch32_write(CH32_REG_OUTPUT, CH32_PIN_SYS_EN | CH32_PIN_LCD_RST | CH32_PIN_TOUCH_RST)) {
            delay(20);
            continue;
        }
        delay(200);
        _expander_ok = true;
    }
    _init_ms = millis() - _init_start;
    // No PWM write here: the factory firmware never touches it and relies on the CH32 default (full brightness)
}

void WaveshareLcd4V4::show_info()
{
    Esp32Device::show_info();
    LOG_INFO(TAG_DEV, F("CH32V003   : %s, %u attempt(s), started at %lu ms, took %lu ms, last i2c err %u"),
             _expander_ok ? "ok" : "FAILED", _init_attempts - 1, _init_start, _init_ms, ch32_last_error);
    LOG_INFO(TAG_DEV, F("CH32V003   : dir=%d out=%d pwm=%d"), ch32_read(CH32_REG_DIRECTION), ch32_read(CH32_REG_OUTPUT),
             ch32_read(CH32_REG_PWM));
}

void WaveshareLcd4V4::set_backlight_pin(uint8_t pin)
{
    // Backlight is driven by the CH32 PWM register, there is no ESP32 pin to attach
    LOG_VERBOSE(TAG_GUI, F("Backlight  : CH32V003 PWM (%s)"), _expander_ok ? "ok" : "not found");
    update_backlight();
}

void WaveshareLcd4V4::set_backlight_invert(bool invert)
{
    _backlight_invert = invert;
    update_backlight();
}

bool WaveshareLcd4V4::get_backlight_invert()
{
    return _backlight_invert;
}

void WaveshareLcd4V4::set_backlight_level(uint8_t level)
{
    _backlight_level = level;
    update_backlight();
}

uint8_t WaveshareLcd4V4::get_backlight_level()
{
    return _backlight_level;
}

void WaveshareLcd4V4::set_backlight_power(bool power)
{
    _backlight_power = power;
    update_backlight();
}

bool WaveshareLcd4V4::get_backlight_power()
{
    return _backlight_power;
}

void WaveshareLcd4V4::update_backlight()
{
    if(!_expander_ok) return;
    // Leave the CH32 default alone until a non-default brightness is requested, like the factory firmware
    if(!_pwm_touched && _backlight_power && _backlight_level == 255) return;
    _pwm_touched = true;

    uint8_t duty = _backlight_power ? map(_backlight_level, 0, 255, 0, CH32_PWM_MAX) : 0;
    if(_backlight_invert) duty = CH32_PWM_MAX - duty;
    bool ok = ch32_write(CH32_REG_PWM, duty);
    LOG_VERBOSE(TAG_GUI, F("Backlight  : CH32V003 PWM %u %s"), duty, ok ? "ok" : "write failed");
}

} // namespace dev

dev::WaveshareLcd4V4 haspDevice;

#endif
