#ifndef _RPC_SERVER_H_
#define _RPC_SERVER_H_

#include <asio.hpp>
#include "protocol.h"

namespace rpc
{
    template <typename Protocol>
    class Server
    {
    public:
        Server(const char *addr, unsigned short port) : ios_(),
                                                        acceptor_(ios_)
        {
        }
        ~Server() {}

        void Start() {}
        void Stop() {}

        template <typename Message>
        void Bind(const typename Message::Handler &handler)
        {
            handler_.Add(Message::Cmd(),
                         Protocol::MessageHandler::Wrap(handler));
        }

    protected:
        asio::io_service ios_;
        asio::ip::tcp::acceptor acceptor_;
        typename Protocol::MessageHandler handler_;

        void OnAccept() {}
    };
}

#endif