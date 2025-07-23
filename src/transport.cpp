#include "net/transport.h"
#include <esp_timer.h>

namespace net
{
    Transport::Transport(const char* tag) :
        mThread("TRANSPORT", 4096, 19),
        mSendQueue(MAX_QUEUE_SIZE),
        mTag(tag)
    {
        // Проверяем инициализацию всех критических компонентов
        if (!mSendQueue.isValid())
        {
            ESP_LOGE(mTag, "Failed to initialize send queue");
            return;
        }
    }

    Transport::~Transport()
    {
        mThread.stop();
        std::lock_guard lock(mMutex);
        mDataCallback.reset();
    }

    void Transport::bind(std::unique_ptr<PacketCallback> dataCallback, PacketErrorFunction errorCallback)
    {
        std::lock_guard lock(mMutex);
        mDataCallback = std::move(dataCallback);
        mErrorCallback = std::move(errorCallback);
    }

    bool Transport::start()
    {
        auto loop = [&]()
        {
            processSendQueue();
            processReceivedData();
            return esp32_c3::objects::Thread::LoopAction::CONTINUE;
        };
        return mThread.quickStart(loop);
    }

    void Transport::stop()
    {
        (void)mSendQueue.reset();
        mThread.stop();
    }

    esp_err_t Transport::send(const Packet& packet)
    {
        std::lock_guard lock(mMutex);

        // Валидация параметров
        if (!isInitialized() || !packet.isValid())
        {
            ESP_LOGE(mTag, "Invalid send params: init=%d, len=%zu, max_mtu=%u",
                     mIsInitialized, packet.size, MAX_MTU);
            return ESP_ERR_INVALID_ARG;
        }

        return mSendQueue.send(packet, 0) ? ESP_OK : ESP_ERR_INVALID_STATE;
    }

    void Transport::processSendQueue()
    {
        if (mNextSendTime > esp_timer_get_time()) return;

        if (Packet packet; mSendQueue.receive(packet, 0))
        {
            if (const esp_err_t ret = sendImpl(packet); ret == ESP_OK)
            {
                mNextSendTime = esp_timer_get_time() + SEND_INTERVAL_US;
                ESP_LOGV(mTag, "Sent successfully");
            }
            else
            {
                handleSendError(packet, ret);
            }
        }
    }

    void Transport::handleSendError(const Packet& packet, const esp_err_t err)
    {
        if (mErrorCallback) mErrorCallback(packet, err);

        if (isTemporary(err))
        {
            ESP_LOGW(mTag, "Temp error (retry): %s", esp_err_to_name(err));
            // Возвращаем обратно в очередь для повторной попытки
            mSendQueue.send(packet, 0);
        }
        else
        {
            ESP_LOGE(mTag, "Fatal error (dropped): %s", esp_err_to_name(err));
        }
    }

    bool Transport::isInitialized() const noexcept
    {
        return mIsInitialized && mThread.state() != esp32_c3::objects::Thread::State::NOT_RUNNING;
    }

    void Transport::setInitialized(const bool value)
    {
        mIsInitialized = value;
    }

    size_t Transport::getQueueSize() const
    {
        return mSendQueue.waiting();
    }

    size_t Transport::clearQueue()
    {
        size_t count = 0;
        Packet packet;
        while (mSendQueue.receive(packet, 0))
        {
            count++;
        }
        return count;
    }

    bool Transport::isTemporary(const esp_err_t ret) noexcept
    {
        return ret == ESP_ERR_NO_MEM ||     // Нехватка памяти
            ret == ESP_ERR_INVALID_STATE || // Временное недопустимое состояние
            ret == ESP_ERR_TIMEOUT;         // Таймаут операции
    }
}
