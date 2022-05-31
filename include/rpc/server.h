#ifndef _RPC_SERVER_H_
#define _RPC_SERVER_H_

#include "session.h"
#include <iostream>

namespace rpc
{
    template <typename Protocol>
    class Server
    {
    public:
        using ServerSession = Session<Protocol>;

        Server(const char *addr, unsigned short port) : endpoint_(asio::ip::address::from_string(addr), port),
                                                        ios_(),
                                                        acceptor_(ios_, endpoint_),
                                                        errorHandler_([](const asio::error_code &) -> void {}),
                                                        handler_(std::make_shared<typename ServerSession::MessageHandler>())
        {
        }
        ~Server() {}

        void Start()
        {
            OnAccept();
            ios_.run();
        }

        void Stop()
        {
            acceptor_.close();
            ios_.stop();
        }

        template <typename Message>
        void Bind(const typename ServerSession::MessageHandler::Handler<Message>::Type &handler)
        {
            handler_->template Bind<Message>(handler);
        }

        inline void SetErrorHandler(const std::function<void(const asio::error_code &)> &handler)
        {
            errorHandler_ = handler;
        }

        asio::io_service &GetIOService() { return ios_; }

    protected:
        asio::ip::tcp::endpoint endpoint_;
        asio::io_service ios_;
        asio::ip::tcp::acceptor acceptor_;
        typename ServerSession::MessageHandler::SharedPtr handler_;
        std::function<void(const asio::error_code &)> errorHandler_;

        void OnAccept()
        {
            std::shared_ptr<ServerSession> session =
                std::make_shared<ServerSession>(handler_, ios_, errorHandler_);
            acceptor_.async_accept(session->Socket(),
                                   [this, session](const asio::error_code &ec) -> void
                                   {
                                       if(ec){
                                           errorHandler_(ec);
                                       }else{
                                           session->Start();
                                           OnAccept();
                                       } });
        }
    };
}

#endif