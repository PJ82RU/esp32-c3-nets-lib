#ifndef NET_UART_H
#define NET_UART_H

#include "transport.h"

#include <driver/gpio.h>
#include <driver/uart.h>

namespace net
{
    /**
     * @brief Константы UART по умолчанию
     */
    constexpr uint32_t UART_DEFAULT_BAUD = 460800; ///< Скорость передачи по умолчанию

    /**
     * @brief Конфигурация UART по умолчанию
     */
    constexpr uart_config_t UART_DEFAULT_CONFIG = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {
            .allow_pd = 0,
            .backup_before_sleep = 0
        }
    };

    /**
     * @brief Тип используемого последовательного порта
     */
    enum class SerialType
    {
        UART0, ///< Порт UART0
        UART1  ///< Порт UART1
    };

    /**
     * @brief Класс для работы с UART интерфейсом
     */
    class Uart final : public Transport
    {
    public:
        /// @brief Тег для логирования
        static constexpr auto TAG = "Uart";

        /**
         * @brief Конструктор UART
         * @param type Тип последовательного порта
         * @param config Конфигурация UART (по умолчанию UART_DEFAULT_CONFIG)
         * @param rxPin Пин RX (опционально)
         * @param txPin Пин TX (опционально)
         */
        explicit Uart(SerialType type,
                      const uart_config_t& config = UART_DEFAULT_CONFIG,
                      std::optional<gpio_num_t> rxPin = std::nullopt,
                      std::optional<gpio_num_t> txPin = std::nullopt) noexcept;
        ~Uart() override;

        // Запрещаем копирование и перемещение
        Uart(const Uart&) = delete;
        Uart(Uart&&) = delete;
        Uart& operator=(const Uart&) = delete;
        Uart& operator=(Uart&&) = delete;

        // Геттеры
        /**
         * @brief Получить тип последовательного порта
         * @return Тип порта
         */
        [[nodiscard]] SerialType type() const noexcept { return mType; }

        /**
         * @brief Получить текущую скорость передачи
         * @return Скорость в бодах
         */
        [[nodiscard]] uint32_t baudRate() const noexcept;

        /**
         * @brief Получить текущий MTU/размер буфера
         * @return size_t Максимальный размер передаваемых данных
         */
        [[nodiscard]] size_t getMtuSize() const noexcept override;

        /**
         * @brief Получить количество доступных байт
         * @return Количество доступных байт
         */
        [[nodiscard]] size_t available() const noexcept;

        /**
         * @brief Прочитать данные в предоставленный буфер
         * @param buffer Буфер для чтения данных (диапазон uint8_t)
         * @return Количество фактически прочитанных байт
         * @note Если буфер пуст или UART не инициализирован, вернёт 0
         */
        [[nodiscard]] size_t read(std::span<uint8_t> buffer) const noexcept;

        /**
         * @brief Записать данные из предоставленного буфера
         * @param data Буфер с данными для записи (диапазон const uint8_t)
         * @return Количество фактически записанных байт
         * @note Если буфер пуст или UART не инициализирован, вернёт 0
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
         * @details Выполняется в бесконечном цикле, пока работает задача
         * @return Никогда не возвращает управление (бесконечный цикл)
         */
        void processReceivedData() override;

    private:
        const SerialType mType; ///< Тип последовательного порта
        uart_port_t mUartNum;   ///< Номер UART порта
    };
} // namespace net

#endif // NET_UART_H
