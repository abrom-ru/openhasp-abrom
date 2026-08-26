# Прошивка ESP32-S3 4848S040 через Flash Download Tool

Инструкция для панели Guition ESP32-S3 4848S040 (16MB flash / 8MB octal PSRAM) с помощью официальной GUI-утилиты Espressif **Flash Download Tool** под Windows.

Используем этот способ, когда:
- нужно прошить с нуля / восстановить «кирпич»;
- не хочется ставить Python и esptool;
- нужно записать **только** прошивку, не трогая раздел LittleFS с настройками.

Для обычных обновлений на живой панели проще OTA — залить `build_output/firmware/esp32-s3-4848s040_ota_v*.bin` через веб-интерфейс панели (`http://<ip>` → Firmware Update).

---

## 1. Подготовка

1. Скачать **Flash Download Tools** с [espressif.com/en/support/download/other-tools](https://www.espressif.com/en/support/download/other-tools) и распаковать.
2. Собрать проект (`pio run -e esp32-s3-4848s040_16MB`) или взять готовые бинарники из `build_output/firmware/`:
   - `bootloader_<hash>.bin`
   - `partitions_<hash>.bin`
   - `firmware_<hash>.bin`
   
   Если этих файлов нет — скопировать из `.pio/build/esp32-s3-4848s040_16MB/`:
   - `bootloader.bin`, `partitions.bin`, `firmware.bin`.
3. Подключить панель по USB-C к тому порту, что помечен как «программируемый» (на 4848S040 порт с CH340, VID:PID `1A86:7523`).

## 2. Запуск утилиты

Открыть `flash_download_tool_x.x.x.exe`. В стартовом окне:

| Поле | Значение |
|---|---|
| ChipType | `ESP32-S3` |
| WorkMode | `Develop` |
| LoadMode | `UART` |

Нажать **OK**.

## 3. Настройка файлов и адресов

В главном окне включить три строки галочками и вписать пути + offset'ы:

| Файл | Offset |
|---|---|
| `bootloader.bin` (или `bootloader_<hash>.bin`) | `0x0` |
| `partitions.bin` | `0x8000` |
| `firmware.bin` | `0x10000` |

## 4. Параметры SPI и порта

Внизу окна:

- **SPI SPEED**: `80 MHz`
- **SPI MODE**: `DIO`
- **DoNotChgBin**: **обязательно включить галочку** ⚠️

  Без этой галочки утилита перезапишет байты `flash_mode` / `flash_size` в заголовке файлов, а SHA-хэш прошивки посчитан по оригинальным байтам — при загрузке получите `Invalid image block, can't boot`.

- **COM**: порт панели (посмотреть в Диспетчере устройств или через `python -m serial.tools.list_ports`).
- **BAUD**: `921600`.

**Кнопку ERASE не нажимать** — она стирает весь флеш, включая раздел LittleFS с настройками, страницами и Wi-Fi. При обычной прошивке она не нужна: `write_flash` перезаписывает только выбранные регионы.

## 5. Прошивка

1. Нажать **START**.
2. Если через 5-10 секунд идёт таймаут / `Failed to connect`:
   - зажать **BOOT** на панели;
   - коротко нажать **RESET**;
   - отпустить **RESET**, потом **BOOT**;
   - снова **START**.
3. Дождаться `FINISH` по всем трём строкам.
4. Нажать **STOP**, отключить USB, включить снова — или коротко нажать RESET.

Панель загрузится: сначала лого ABROM, потом либо OOBE-мастер (при чистой ФС), либо привычный интерфейс. Если ФС не была стёрта — все настройки останутся.

## 6. Проверка через монитор

Открыть Serial Monitor на том же COM-порту, `115200 8N1`. Признак успешной загрузки — сообщения `[HASP]` / `[WIFI]` / `[MQTT]` вместо `Invalid image block`.

---

## Альтернатива: esptool.py (командная строка)

Тот же результат без GUI, работает из bash/PowerShell. Использует Python и esptool из PlatformIO — глобальная установка не нужна:

```bash
~/.platformio/penv/Scripts/python.exe \
  ~/.platformio/packages/tool-esptoolpy/esptool.py \
  --chip esp32s3 --port COM4 --baud 921600 \
  --before default_reset --after hard_reset \
  write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m \
  0x0     build_output/firmware/bootloader_<hash>.bin \
  0x8000  build_output/firmware/partitions_<hash>.bin \
  0x10000 build_output/firmware/firmware_<hash>.bin
```

esptool заголовки не трогает никогда — эквивалент включённого `DoNotChgBin`.

## Типичные ошибки

| Симптом | Причина | Что делать |
|---|---|---|
| `Could not open COM4, the port is busy` | Порт держит другой процесс (Serial Monitor, PuTTY, ещё одна копия FDT) | Закрыть всё, что открывает COM-порт |
| `Failed to connect ... Wrong boot mode` | Чип не в bootloader mode | Зажать BOOT → RESET → отпустить RESET → отпустить BOOT → START |
| `Invalid image block, can't boot` в мониторе | Заголовок бинарника был пропатчен утилитой | Перезалить с включённым `DoNotChgBin` |
| Экран чёрный, но в логе `[HASP]` идёт | Прошивка ок, но питание/подсветка дисплея вырублены | Проверить настройки backlight / GPIO дисплея |
