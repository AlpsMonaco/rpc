#include "include/rpc/server.h"
#include "include/rpc/session.h"
#include "include/rpc/client.h"
#include <iostream>
#include <string>

// All relevant classes is implemented with rpc::Protocol.
using Protocol = rpc::Protocol<unsigned short, unsigned short>;
using Server = rpc::Server<Protocol>;
using Client = rpc::Client<Protocol>;
using Session = rpc::Session<Protocol>;

struct Foo1
{
    int i;
    double d;
};

struct Foo2
{
    int i;
    unsigned short textSize;
    char text[120];

    struct SizeGetter
    {
        unsigned short operator()(const Foo2 &foo) const
        {
            return sizeof(foo.i) + sizeof(foo.textSize) + foo.textSize;
        }
    };
};

// wrap your own class with Protocol::MessageWrapper.

// Use sizeof() to calculate wrapped struct by default.
using MsgFoo1 = Protocol::MessageWrapper<0x01, Foo1>;

// If the size of wrapped struct is variable,you could specify a SizeGetter to calculate the message.
using MsgFoo2 = Protocol::MessageWrapper<0x02, Foo2, Foo2::SizeGetter>;

int main(int argc, char **argv)
{
    const char *const IP = "127.0.0.1";
    unsigned short port = 8890;

    Server server(IP, port);
    server.SetErrorHandler([](const asio::error_code &ec) -> void
                           { std::cout << ec << " " << ec.message() << std::endl; });
    server.Bind<MsgFoo1>(
        [](const MsgFoo1 &req, Session &session) -> void
        {
            std::cout << req->i << std::endl;
            std::cout << req->d << std::endl;

            // every session have a buffer which is large to max protocol size.
            // also it is concurrent safe.
            MsgFoo2 &ret2 = session.WriteBuffer();
            ret2->i = 3;
            static const char v[] = "Hello";
            strcpy(ret2->text, v);
            ret2->textSize = strlen(v);

            // only class with Bytes() convert could be sent.
            // all classes/struct wrapped by MessageWrapper has converter by default.
            session.Write(ret2);
        });
    std::thread serverThread([&]() -> void
                             { server.Start(); });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    Client client(IP, port);
    client.SetErrorHandler([](const asio::error_code &ec) -> void
                           { std::cout << ec << " " << ec.message() << std::endl; });
    client.Bind<MsgFoo2>([](const MsgFoo2 &req, Session &session) -> void
                         {
            std::string_view sv(req->text, req->textSize);
            std::cout << sv << std::endl; });

    std::thread clientThread([&]() -> void
                             { client.Start(); });
    MsgFoo1 &msg = client.WriteBuffer();
    msg->i = 1;
    msg->d = 2;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    client.Send(msg);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    server.Stop();
    client.Close();
    clientThread.join();
    serverThread.join();
}