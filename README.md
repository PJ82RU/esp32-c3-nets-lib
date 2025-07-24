# ESP32-C3 Nets Library

![Лицензия](https://img.shields.io/badge/license-Unlicense-blue.svg)
![Платформа](https://img.shields.io/badge/platform-ESP32--C3-orange.svg)
![Версия](https://img.shields.io/badge/version-1.0.0-green.svg)

![BLE 5.0](https://img.shields.io/badge/BLE-5.0%2F4.2-brightgreen.svg)
![UART](https://img.shields.io/badge/UART-Up_to_460800_baud-blue.svg)
![USB-JTAG](https://img.shields.io/badge/USB--JTAG-1.5MB_buffer-orange.svg)

Унифицированная библиотека для сетевого взаимодействия на ESP32-C3, поддерживающая интерфейсы BLE 5.0, UART и USB-JTAG с
общим пакетным протоколом.

## Возможности

- **Кросс-интерфейсная совместимость**: Единый формат пакетов для всех поддерживаемых транспортов
- **Bluetooth Low Energy 5.0**:
    - Поддержка расширенной рекламы (BLE 5.0)
    - Настраиваемые режимы PHY (1M, 2M, Coded)
    - Управление подключениями с согласованием MTU
- **UART**:
    - Поддержка высокоскоростной передачи (до 460800 бод)
    - Гибкая конфигурация параметров порта
    - Буферизованная очередь отправки
- **USB-JTAG**:
    - Использование интерфейса отладки для передачи данных
    - Автоматическое определение подключения
- **Общие функции**:
    - Потокобезопасные очереди отправки
    - Система callback-ов для обработки входящих данных
    - Единый API для всех интерфейсов

## Поддерживаемые платформы

- ESP32-C3 (все модели)

## Установка

1. Через PlatformIO:

```ini
lib_deps =
    PJ82RU/esp32-c3-nets-lib@^1.0.0
```

2. Вручную:

```bash
git clone https://github.com/PJ82RU/esp32-c3-nets-lib.git
```

## Быстрый старт

```cpp
#include <net/nets.h>

// Создание BLE интерфейса
auto ble = std::make_unique<net::BLE>("MyDevice");

// Инициализация
if (ble->quickStart([](net::Packet packet) {
    // Обработка входящих данных
    ESP_LOGI("BLE", "Получен пакет: %s", packet.headerInfo().c_str());
}) != ESP_OK) {
    ESP_LOGE("BLE", "Ошибка инициализации!");
}
```

## Лицензия

Данная библиотека распространяется под [лицензией Unlicense](https://unlicense.org/).