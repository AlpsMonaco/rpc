#ifndef _RPC_SESSION_H_
#define _RPC_SESSION_H_

#include "protocol.h"
#include <asio.hpp>
#include <fstream>

namespace rpc
{

    template <typename Protocol>
    class Session
        : public std::enable_shared_from_this<Session<Protocol>>
    {
    public:
        using MessageHandler = typename Protocol::template MessageHandler<Session>;

        Session(
            const typename MessageHandler::SharedPtr &handler,
            asio::io_service &ios,
            const std::function<void(const asio::error_code &)> &errorHandler = [](const asio::error_code &) -> void {})
            : handler_(handler),
              socket_(ios),
              errorHandler_(errorHandler),
              buffer_(),
              writeBuffer_(),
              readSize_(0),
              extraOffset_(0),
              remainSize_(0)
        {
        }

        ~Session() {}

        inline void Start()
        {
            OnRead();
        }
        inline void Close() { socket_.close(); }

        typename Protocol::Buffer &WriteBuffer() { return writeBuffer_; }

        void Write(const typename Protocol::Bytes &bytes)
        {
            asio::async_write(
                socket_,
                asio::buffer(bytes.Data(), bytes.Size()),
                [this](const asio::error_code &ec, size_t) -> void
                {
                    if (ec)
                        errorHandler_(ec);
                });
        }

        void Send(const typename Protocol::Bytes &bytes)
        {
            asio::async_write(
                socket_,
                asio::buffer(bytes.Data(), bytes.Size()),
                [this](const asio::error_code &ec, size_t) -> void
                {
                    if (ec)
                        errorHandler_(ec);
                });
        }

        asio::ip::tcp::socket &Socket() { return socket_; }

        inline void SetErrorHandler(const std::function<void(const asio::error_code &)> &handler)
        {
            errorHandler_ = handler;
        }

        std::string_view HandshakeMessage() { return "CPP RPC SESSION PROTOCOL"; }

    protected:
        asio::ip::tcp::socket socket_;
        typename MessageHandler::SharedPtr handler_;
        typename Protocol::Buffer buffer_;
        typename Protocol::Buffer writeBuffer_;
        size_t readSize_;
        size_t remainSize_;
        size_t extraOffset_;
        std::function<void(const asio::error_code &)> errorHandler_;

        inline char *GetNextBuffer() { return buffer_.Get() + readSize_; }
        inline size_t GetNextSize() { return Protocol::maxSize - readSize_; }
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
            (*handler_)[packet.cmd](packet, *this);
            if (extraOffset_ > 0)
                ParseExtraData();
        }

        void ParseExtraData()
        {
            do
            {
                remainSize_ = readSize_ - extraOffset_;
                if (remainSize_ < Protocol::headerLength)
                {
                    memcpy(buffer_.Get(), buffer_.Get() + extraOffset_, remainSize_);
                    readSize_ = remainSize_;
                    extraOffset_ = 0;
                    return;
                }
                const typename Protocol::Packet &packet = Protocol::Packet::Cast(buffer_.Get() + extraOffset_);
                if (remainSize_ < packet.size)
                {
                    memcpy(buffer_.Get(), buffer_.Get() + extraOffset_, remainSize_);
                    extraOffset_ = 0;
                    readSize_ = remainSize_;
                    return;
                }
                if (remainSize_ == packet.size)
                {
                    readSize_ = 0;
                    extraOffset_ = 0;
                }
                else
                    extraOffset_ += packet.size;
                (*handler_)[packet.cmd](packet, *this);
            } while (extraOffset_ > 0);
        }

        void OnRead()
        {
            auto self(this->shared_from_this());
            socket_.async_read_some(
                asio::buffer(GetNextBuffer(), GetNextSize()),
                [&, self](const asio::error_code &ec, size_t bytes) -> void
                {
                    if (ec)
                        errorHandler_(ec);
                    else
                    {
                        ReadBytes(bytes);
                        OnRead();
                    }
                });
        }
    };
}
#endif