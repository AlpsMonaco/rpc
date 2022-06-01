#ifndef _RPC_CLIENT_H_
#define _RPC_CLIENT_H_

#include "session.h"

namespace rpc
{

    template <typename Protocol>
    class Client
    {
    public:
        using ClientSession = Session<Protocol>;

        Client(const char *ip, unsigned short port) : endpoint_(asio::ip::address::from_string(ip), port),
                                                      ios_(),
                                                      msgHandler_(std::make_shared<typename ClientSession::MessageHandler>()),
                                                      errorHandler_([](const asio::error_code &) -> void {}),
                                                      session_(std::make_shared<ClientSession>(msgHandler_, ios_, errorHandler_))
        {
        }
        ~Client() {}

        void Start()
        {
            session_->Socket().async_connect(endpoint_, [this](const asio::error_code &ec) -> void
                                             {
            if (ec) {
                errorHandler_(ec);
            } else {
                session_->Start();
            } });
            ios_.run();
        }

        inline void Close()
        {
            session_->Close();
            ios_.stop();
        }

        inline void SetErrorHandler(const std::function<
                                    void(const asio::error_code &)> &handler)
        {
            errorHandler_ = handler;
            session_->SetErrorHandler(handler);
        }

        template <typename Message>
        void Bind(const typename ClientSession::MessageHandler::template Handler<Message>::Type &handler)
        {
            msgHandler_->template Bind<Message>(handler);
        }

        void Send(const typename Protocol::Bytes &bytes)
        {
            session_->Send(bytes);
        }

        inline typename Protocol::Buffer &WriteBuffer() { return session_->WriteBuffer(); }

    protected:
        asio::ip::tcp::endpoint endpoint_;
        asio::io_service ios_;
        typename ClientSession::MessageHandler::SharedPtr msgHandler_;
        std::function<void(const asio::error_code &)> errorHandler_;
        std::shared_ptr<ClientSession> session_;
    };
}
#endif