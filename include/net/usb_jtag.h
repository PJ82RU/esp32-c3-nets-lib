#ifndef NET_USB_JTAG_H
#define NET_USB_JTAG_H

#include "transport.h"

namespace net
{
    /**
     * @brief Класс для работы с USB-JTAG интерфейсом
     */
    class UsbJtag final : public Transport
    {
    public:
        /// @brief Тег для логирования
        static constexpr auto TAG = "UsbJtag";

        static constexpr size_t USB_JTAG_TX_BUFFER_SIZE = 1024;  // 1KB TX (2 пакета + резерв)
        static constexpr size_t USB_JTAG_RX_BUFFER_SIZE = 1536;  // 1.5KB RX (3 пакета)

        /**
         * @brief Конструктор USB-JTAG
         */
        explicit UsbJtag() noexcept;
        ~UsbJtag() override;

        // Запрещаем копирование и перемещение
        UsbJtag(const UsbJtag&) = delete;
        UsbJtag(UsbJtag&&) = delete;
        UsbJtag& operator=(const UsbJtag&) = delete;
        UsbJtag& operator=(UsbJtag&&) = delete;

        /**
         * @brief Получить текущий MTU/размер буфера
         * @return size_t Максимальный размер передаваемых данных
         */
        [[nodiscard]] size_t getMtuSize() const noexcept override;

        /**
         * @brief Прочитать данные в предоставленный буфер
         * @param buffer Буфер для чтения данных (диапазон uint8_t)
         * @return Количество фактически прочитанных байт
         * @note Если буфер пуст или USB-JTAG не инициализирован, вернёт 0
         */
        [[nodiscard]] size_t read(std::span<uint8_t> buffer) const noexcept;

        /**
         * @brief Записать данные из предоставленного буфера
         * @param data Буфер с данными для записи (диапазон const uint8_t)
         * @return Количество фактически записанных байт
         * @note Если буфер пуст или USB-JTAG не инициализирован, вернёт 0
         */
        [[nodiscard]] size_t write(std::span<const uint8_t> data) const noexcept;

    protected:
        /**
         * @brief Отправить пакет данных
         * @param packet Ссылка на пакет для отправки
         * @return esp_err_t Код ошибки ESP-IDF
         */
        [[nodiscard]] esp_err_t sendImpl(Packet& packet) override;

        /**
         * @brief Обработка принятых данных
         */
        void processReceivedData() override;
    };
} // namespace net

#endif // NET_USB_JTAG_H