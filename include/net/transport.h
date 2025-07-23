#ifndef NET_TRANSPORT_H
#define NET_TRANSPORT_H

#include "net/packet.h"
#include "esp32_c3_objects/callback.h"

#include <memory>
#include <mutex>

namespace net
{
    /**
     * @brief Базовый класс для всех транспортных интерфейсов (BLE, UART, USB-JTAG)
     */
    class Transport
    {
    public:
        static constexpr uint32_t SEND_INTERVAL_US = 20 * 1000; ///< Интервал между отправками (20 мс)
        static constexpr size_t MAX_QUEUE_SIZE = 16;            ///< Максимальный размер очереди отправки

        using PacketCallback = esp32_c3::objects::Callback<Packet>;
        using PacketErrorFunction = std::function<void(const Packet& packet, esp_err_t ret)>;
        using PacketQueue = esp32_c3::objects::BufferedQueue<Packet, MAX_QUEUE_SIZE>;

        virtual ~Transport();

        // Запрет копирования и присваивания
        Transport(const Transport&) = delete;
        Transport& operator=(const Transport&) = delete;

        /**
         * @brief Привязать callback для обработки входящих данных
         * @param dataCallback Уникальный указатель на callback для входящих данных
         * @param errorCallback Уникальный указатель на callback для ошибок отправки
         */
        void bind(std::unique_ptr<PacketCallback> dataCallback, PacketErrorFunction errorCallback = nullptr);

        /**
         * @brief Запуск/проверка рабочего потока
         * @return true если поток успешно запущен или уже работает
         */
        bool start();

        /**
         * @brief Остановка рабочего потока
         */
        virtual void stop();

        /**
         * @brief Проверить, инициализирован ли транспорт
         * @return bool true если инициализирован
         */
        [[nodiscard]] bool isInitialized() const noexcept;

        /**
         * @brief Получить текущий MTU/размер буфера
         * @return size_t Максимальный размер передаваемых данных
         */
        [[nodiscard]] virtual size_t getMtuSize() const noexcept = 0;

        /**
         * @brief Добавить пакет в очередь отправки (потокобезопасно)
         * @param packet Пакет для отправки
         * @return esp_err_t ESP_OK при успешном добавлении в очередь
         */
        esp_err_t send(const Packet& packet);

        /**
         * @brief Получить текущий размер очереди отправки
         * @return size_t Количество пакетов в очереди
         */
        [[nodiscard]] size_t getQueueSize() const;

        /**
         * @brief Очистить очередь отправки
         * @return size_t Количество удалённых пакетов
         */
        size_t clearQueue();

    protected:
        explicit Transport(const char* tag);

        /**
         * @brief Внутренняя реализация отправки пакета
         * @param packet Пакет для отправки
         * @return esp_err_t Результат отправки
         * @note Должна быть переопределена в наследниках
         */
        [[nodiscard]] virtual esp_err_t sendImpl(Packet& packet) = 0;

        /**
         * @brief Обработать очередь отправки
         * @note Отправляет не чаще чем раз в SEND_INTERVAL_US
         */
        void processSendQueue();

        /**
         * @brief Обрабатывает ошибки отправки пакета согласно стратегии восстановления
         */
        void handleSendError(const Packet& packet, esp_err_t err);

        /**
         * @brief Обработка входящих данных (для транспортов с непрерывным потоком)
         * @note Для BLE не используется (обработка через callback)
         */
        virtual void processReceivedData() {};

        /**
         * @brief Устанавливает флаг инициализации транспорта
         * @param value Новое состояние флага (true - инициализирован, false - не инициализирован)
         */
        void setInitialized(bool value);

        mutable std::recursive_mutex mMutex; ///< Мьютекс для потокобезопасности
        esp32_c3::objects::Thread mThread;   ///< Поток обработки

        std::unique_ptr<PacketCallback> mDataCallback; ///< Callback для данных
        PacketErrorFunction mErrorCallback;            ///< Callback для ошибок отправки
        PacketQueue mSendQueue;                        ///< Очередь пакетов на отправку

        uint64_t mNextSendTime = 0; ///< Время следующей отправки (мкс)

    private:
        /**
         * @brief Проверка, является ли ошибка временной
         * @return true если ошибка допускает повторную отправку
         */
        [[nodiscard]] static bool isTemporary(esp_err_t ret) noexcept;

        const char* mTag;            ///< Тег для логирования
        bool mIsInitialized = false; ///< Флаг инициализации
    };
} // namespace net

#endif // NET_TRANSPORT_H
