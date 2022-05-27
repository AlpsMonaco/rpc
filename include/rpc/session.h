#ifndef _RPC_SESSION_H_
#define _RPC_SESSION_H_

#include "protocol.h"
#include <asio.hpp>

namespace rpc
{

    template <typename Protocol>
    class Session
        : public std::enable_shared_from_this<Session<Protocol>>
    {
    public:
        using MessageHandler = typename Protocol::MessageHandler<Session>;

        Session(
            const typename MessageHandler::SharedPtr &handler,
            asio::io_service &ios,
            const std::function<void(const asio::error_code &ec)> &errorHandler = [](const asio::error_code &ec) -> void {})
            : handler_(handler),
              socket_(ios),
              errorHandler_(errorHandler)
        {
        }

        ~Session() {}

        inline void Start() { OnRead(); }
        inline void Close() { socket_.close(); }

        asio::ip::tcp::socket &Socket() { return socket_; }

    protected:
        asio::ip::tcp::socket socket_;
        typename MessageHandler::SharedPtr handler_;
        typename Protocol::Buffer buffer_;
        size_t readSize_;
        size_t extraOffset_;
        std::function<void(const asio::error_code &ec)> errorHandler_;

        inline char *GetNextBuffer() { return buffer_.Get() + readSize_; }
        inline size_t GetNextSize() { return Protocol::PacketSize - readSize_; }
        inline const char *Data() { return buffer_.Get(); }

        void ReadBytes(size_t readSize)
        {
            readSize_ += readSize;
            if (readSize_ < Protocol::headerLength)
                return;
            const typename Protocol::Packet &packet = Protocol::Packet::Cast(buffer_.Get());
            if (readSize_ < packet.size)
                return;
            if (readSize_ == packet.size)
                readSize_ = 0;
            else
                extraOffset_ = packet.size;
            (*handler_)[packet.cmd](packet);
            if (extraOffset_ > 0)
                ParseExtraData();
        }

        void ParseExtraData()
        {
            static size_t remainSize = 0;
            do
            {
                remainSize = readSize_ - extraOffset_;
                if (remainSize < Protocol::headerLength)
                {
                    memcpy(buffer_.Get(), buffer_.Get() + extraOffset_, remainSize);
                    readSize_ = remainSize;
                    extraOffset_ = 0;
                    return;
                }
                const typename Protocol::Packet &packet = Protocol::Packet::Cast(buffer_.Get());
                if (remainSize < packet.size)
                {
                    memcpy(buffer_.Get(), buffer_.Get() + extraOffset_, remainSize);
                    extraOffset_ = 0;
                    readSize_ = remainSize;
                    return;
                }
                if (remainSize == packet.size)
                {
                    readSize_ = 0;
                    extraOffset_ = 0;
                }
                else
                    extraOffset_ += packet.size;
                (*handler_)[packet.cmd](packet);
            } while (extraOffset_ > 0);
        }

        void OnRead()
        {
        }
    };
}
#endif