#ifndef _RPC_SERVER_H_
#define _RPC_SERVER_H_

#include <asio.hpp>

namespace rpc
{
    template <typename Protocol>
    class Server
    {
    public:
        Server(const char *addr, unsigned short port);
        ~Server();

        void Start();
        void Stop();

    protected:
        asio::io_service ios_;
        asio::ip::tcp::acceptor acceptor_;

        void OnAccept();
    };
}

#endif