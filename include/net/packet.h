#ifndef NET_PACKET_H
#define NET_PACKET_H

#include <cstdint>
#include <cstring>
#include <string>
#include <array>
#include <esp_err.h>

namespace net
{
    /**
     * @file packet.h
     * @brief Заголовочный файл структуры пакета для BLE/UART通信
     */

    /// @brief Максимально возможный размер MTU для BLE 5.0
    constexpr uint16_t MAX_MTU = 517;

#pragma pack(push, 1) // Выравнивание по 1 байту для совместимости

    /**
     * @struct Packet
     * @brief Универсальная структура пакета данных
     *
     * @details Используется для передачи данных между устройствами:
     * - По Bluetooth Low Energy (BLE)
     * - Через последовательный порт (UART)
     *
     * @warning Для кросс-платформенной совместимости:
     * - Запрещено менять порядок полей
     * - Запрещено изменять размер полей
     */
    struct Packet
    {
        /**
         * @brief Идентификатор отправителя/соединения
         * @details Используется для:
         * - BLE: ID соединения (conn_id)
         * - UART: номер устройства
         * @note 0 означает широковещательное сообщение
         */
        uint16_t id = 0;

        /**
         * @brief Реальный размер данных в пакете
         * @details Должен удовлетворять условию:
         * 0 < size <= PACKET_DATA_SIZE
         * @warning Данные за пределами size считаются недействительными
         */
        uint16_t size = 0;

        /**
         * @brief Буфер для хранения данных
         * @details Особенности использования:
         * - Для текста: должен включать нуль-терминатор
         * - Для бинарных данных: только значимые байты
         */
        std::array<uint8_t, MAX_MTU> buffer{};

        /**
         * @brief Проверка корректности пакета
         * @return true - пакет валиден
         * @return false - пакет поврежден
         */
        [[nodiscard]] bool isValid() const noexcept
        {
            return size > 0 && size <= MAX_MTU;
        }

        /**
         * @brief Строковая информация о пакете
         * @return Строка вида "Packet[id=1, size=128, valid=true]"
         */
        [[nodiscard]] std::string headerInfo() const
        {
            return "Packet[id=" + std::to_string(id) +
                ", size=" + std::to_string(size) +
                ", valid=" + (isValid() ? "true" : "false") + "]";
        }

        /**
         * @brief Очистка пакета
         * @post Все поля обнулены, пакет невалиден
         */
        void clear() noexcept
        {
            id = 0;
            size = 0;
            buffer.fill(0);
        }

        /**
         * @brief Запись данных в пакет
         * @param data Указатель на данные
         * @param len Длина данных
         * @return true - данные записаны
         * @return false - ошибка параметров
         */
        bool setPayload(const uint8_t* data, const size_t len) noexcept
        {
            if (!data || len == 0 || len > MAX_MTU)
                return false;

            size = static_cast<uint16_t>(len);
            std::memcpy(buffer.data(), data, len);
            return true;
        }
    };

#pragma pack(pop) // Восстановление выравнивания

    // Проверка размера структуры
    static_assert(sizeof(Packet) == 2 + 2 + MAX_MTU,
                  "Некорректный размер структуры Packet");
}

#endif // NET_PACKET_H
