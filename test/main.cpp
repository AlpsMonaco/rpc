#include "include/rpc/server.h"
#include <iostream>

using namespace std;

using Protocol = rpc::Protocol<unsigned short, unsigned short>;
using Server = rpc::Server<Protocol>;

struct Foo
{
    unsigned short i;
    unsigned long j;
};

using MsgFoo = Protocol::MessageWrapper<1, Foo>;

void PrintFoo(const MsgFoo &foo)
{
    cout << foo->i << endl;
    cout << foo->j << endl;
}

void Bar(const Protocol::Bytes &b)
{
    for (int i = 0; i < b.Size(); i++)
    {
        cout << static_cast<unsigned int>(static_cast<unsigned char>(b.Data()[i])) << endl;
    }
}

int main(int argc, char **argv)
{
    Server server("127.0.0.1", 8888);
    server.Bind<MsgFoo>(PrintFoo);
    server.Start();
    MsgFoo msgFoo;
    msgFoo->i = 1;
    msgFoo->j = 2;
    PrintFoo(msgFoo);
    Bar(msgFoo);
    Foo &foo = msgFoo;
}