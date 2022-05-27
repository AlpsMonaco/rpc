#include "include/rpc/server.h"
#include "include/rpc/session.h"
#include <iostream>
#include <string_view>
#include <memory>

using namespace std;

using Protocol = rpc::Protocol<unsigned short, unsigned short>;
using Server = rpc::Server<Protocol>;
using Session = rpc::Session<Protocol>;

struct FooString
{
    long long i;
    unsigned char str_size;
    char str[255];

    struct SizeGetter
    {
        Protocol::Size operator()(const FooString &t) const { return sizeof(t.i) + sizeof(t.str_size) + t.str_size; }
    };
};

using MsgFooString = Protocol::MessageWrapper<0x01, FooString, FooString::SizeGetter>;

int main()
{
    Server server("127.0.0.1", 12345);
    server.Bind<MsgFooString>(
        [](const MsgFooString &msg, Session &session) -> void
        {
            std::cout << msg->i << std::endl;
            std::cout << std::string_view(msg->str, msg->str_size) << std::endl;
            MsgFooString &ret = session.WriteBuffer();
            ret = msg;
            session.Write(ret);
        });
}