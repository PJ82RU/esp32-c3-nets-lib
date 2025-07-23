#include "net/uart.h"
#include <esp_log.h>
#include <driver/uart.h>

namespace net
{
    Uart::Uart(const SerialType type,
               const uart_config_t& config,
               const std::optional<gpio_num_t> rxPin,
               const std::optional<gpio_num_t> txPin) noexcept :
        Transport(TAG),
        mType(type),
        mUartNum(type == SerialType::UART0 ? UART_NUM_0 : UART_NUM_1)
    {
        ESP_LOGI(TAG, "Initializing UART%d, baud: %u", mUartNum, config.baud_rate);

        esp_err_t ret = uart_param_config(mUartNum, &config);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to configure UART%d: %s", mUartNum, esp_err_to_name(ret));
            return;
        }

        if (rxPin && txPin)
        {
            ret = uart_set_pin(mUartNum, *txPin, *rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to set UART%d pins: %s", mUartNum, esp_err_to_name(ret));
                return;
            }
        }

        ret = uart_driver_install(mUartNum, MAX_MTU, MAX_MTU, 0, nullptr, 0);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to install UART%d driver: %s", mUartNum, esp_err_to_name(ret));
            return;
        }

        setInitialized(true);
        ESP_LOGI(TAG, "UART%d initialized successfully", mUartNum);
    }

    Uart::~Uart()
    {
        if (!isInitialized()) return;

        if (const esp_err_t ret = uart_driver_delete(mUartNum); ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to delete UART%d driver: %s", mUartNum, esp_err_to_name(ret));
        }
    }

    uint32_t Uart::baudRate() const noexcept
    {
        std::lock_guard lock(mMutex);
        if (!isInitialized()) return 0;

        uint32_t rate = 0;
        if (const esp_err_t ret = uart_get_baudrate(mUartNum, &rate); ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get baud rate: %s", esp_err_to_name(ret));
        }
        ESP_LOGD(TAG, "Baud rate: %" PRIu32, rate);
        return rate;
    }

    size_t Uart::getMtuSize() const noexcept
    {
        return MAX_MTU;
    }

    size_t Uart::available() const noexcept
    {
        std::lock_guard lock(mMutex);
        if (!isInitialized()) return 0;

        size_t avail = 0;
        if (const esp_err_t ret = uart_get_buffered_data_len(mUartNum, &avail); ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get available bytes: %s", esp_err_to_name(ret));
        }
        ESP_LOGV(TAG, "Available bytes: %zu", avail);
        return avail;
    }

    size_t Uart::read(std::span<uint8_t> buffer) const noexcept
    {
        std::lock_guard lock(mMutex);
        if (!isInitialized() || buffer.empty())
        {
            ESP_LOGW(TAG, "Invalid read parameters");
            return 0;
        }

        ESP_LOGD(TAG, "Reading %zu bytes", buffer.size());
        const int len = uart_read_bytes(mUartNum, buffer.data(), buffer.size(), pdMS_TO_TICKS(100));
        if (len < 0)
        {
            ESP_LOGE(TAG, "Failed to read bytes");
            return 0;
        }
        ESP_LOGV(TAG, "Read %d bytes", len);
        return static_cast<size_t>(len);
    }

    size_t Uart::write(const std::span<const uint8_t> data) const noexcept
    {
        // std::lock_guard lock(mMutex);
        if (!isInitialized() || data.empty())
        {
            ESP_LOGE(TAG, "Invalid write parameters");
            return 0;
        }

        ESP_LOGD(TAG, "Writing %zu bytes", data.size());
        const int len = uart_write_bytes(mUartNum, data.data(), data.size());
        if (len < 0)
        {
            ESP_LOGE(TAG, "Failed to write bytes");
            return 0;
        }
        ESP_LOGV(TAG, "Wrote %d bytes", len);
        return static_cast<size_t>(len);
    }

    esp_err_t Uart::sendImpl(Packet& packet)
    {
        ESP_LOGD(TAG, "Writing packet, size: %zu", packet.size);
        const size_t written = write(std::span(packet.buffer.data(), packet.size));

        if (written != packet.size)
        {
            ESP_LOGE(TAG, "Failed to write full packet: %zu/%zu bytes", written, packet.size);
            return ESP_FAIL;
        }

        ESP_LOGV(TAG, "Packet write success: %zu bytes", written);
        return ESP_OK;
    }

    void Uart::processReceivedData()
    {
        if (!mDataCallback || available() == 0) return;

        Packet packet{};
        packet.size = read(std::span<uint8_t>(packet.buffer));

        if (packet.size > 0)
        {
            ESP_LOGV(TAG, "Processing %zu bytes", packet.size);
            mDataCallback->invoke(packet, [&](const Packet& result)
            {
                send(result);
            });
        }
    }
} // namespace net
