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

    Server::Server(const char *addr, unsigned short port) : ios_(),
                                                            acceptor_(ios_, tcp::endpoint(address::from_string(addr), port))
    {
    }

    ~Server() {}

    void Server::Start()
    {
        OnAccept();
        ios_.run();
    }

    void Server::Stop() {}

    void Server::OnAccept()
    {
        asio::ip::tcp::socket socket;
        acceptor_.async_accept(socket, [&](const asio::error_code &ec) -> void
                               {
                if(ec){
                    //todo: handler ec
                    return;
                } 
                // todo: init socket
                OnAccept(); });
    }
}

#endif