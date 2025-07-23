#include "net/usb_jtag.h"

#include <esp_log.h>
#include <driver/usb_serial_jtag.h>

namespace net
{
    UsbJtag::UsbJtag() noexcept : Transport(TAG)
    {
        ESP_LOGI(TAG, "Initializing USB-JTAG interface");

        // Конфигурация драйвера USB-JTAG
        usb_serial_jtag_driver_config_t config = {
            .tx_buffer_size = USB_JTAG_TX_BUFFER_SIZE,
            .rx_buffer_size = USB_JTAG_RX_BUFFER_SIZE
        };

        // Инициализация драйвера USB-JTAG
        if (const esp_err_t ret = usb_serial_jtag_driver_install(&config); ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to install USB-JTAG driver: %s", esp_err_to_name(ret));
            return;
        }

        setInitialized(true);
        ESP_LOGI(TAG, "USB-JTAG initialized. Buffers: TX=%zu, RX=%zu",
                 USB_JTAG_TX_BUFFER_SIZE, USB_JTAG_RX_BUFFER_SIZE);
    }

    UsbJtag::~UsbJtag()
    {
        if (isInitialized())
        {
            usb_serial_jtag_driver_uninstall();
            ESP_LOGI(TAG, "Driver uninstalled");
        }
    }

    size_t UsbJtag::getMtuSize() const noexcept
    {
        return MAX_MTU;
    }

    size_t UsbJtag::read(std::span<uint8_t> buffer) const noexcept
    {
        std::lock_guard lock(mMutex);
        if (!isInitialized() || buffer.empty())
        {
            ESP_LOGW(TAG, "Invalid read parameters");
            return 0;
        }

        ESP_LOGD(TAG, "Reading %zu bytes", buffer.size());
        const int len = usb_serial_jtag_read_bytes(buffer.data(), buffer.size(), pdMS_TO_TICKS(50));
        if (len < 0)
        {
            ESP_LOGE(TAG, "Failed to read bytes");
            return 0;
        }
        ESP_LOGV(TAG, "Read %d bytes", len);
        return static_cast<size_t>(len);
    }

    size_t UsbJtag::write(const std::span<const uint8_t> data) const noexcept
    {
        if (!isInitialized() || data.empty())
        {
            ESP_LOGE(TAG, "Invalid write parameters");
            return 0;
        }

        ESP_LOGD(TAG, "Writing %zu bytes", data.size());
        const int len = usb_serial_jtag_write_bytes(data.data(), data.size(), pdMS_TO_TICKS(100));
        if (len < 0)
        {
            ESP_LOGE(TAG, "Failed to write bytes");
            return 0;
        }
        ESP_LOGV(TAG, "Wrote %d bytes", len);
        return static_cast<size_t>(len);
    }

    esp_err_t UsbJtag::sendImpl(Packet& packet)
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

    void UsbJtag::processReceivedData()
    {
        if (!mDataCallback) return;

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
