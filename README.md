# rpc
A header only and out of the box tcp rpc framework written in C++.

## Introduction

### Protocol
Protocol is a template specified on compile time.  
Server,Client and Session are all based on it.  

```c++
using SizeType = unsigned short;
using CmdType = unsigned short;
using Protocol = rpc::Protocol<SizeType, CmdType>;
```

**notice**: SizeType should be unsigned.If not,there will be a assert on debug mode and the program will be abort.  


### Message
All message communicates on this framework should be wrapped by rpc::Protocol::MessageWrapper.  
Otherwise the compile will be failed.  
```c++
template <typename Protocol::Cmd cmd, typename MessageType,
                  class MessageSizeGetter = DefaultMessageSizeGetter<MessageType, typename Protocol::Size>> //optional
```




Wrap your class with Cmd,Struct and SizeGetter(optional)  
```c++
struct Foo1
{
    int i;
    double d;
};

// Use sizeof() to calculate wrapped struct by default if you do not specify SizeGetter.
using MsgFoo1 = Protocol::MessageWrapper<0x01, Foo1>;

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

// If the size of wrapped struct is variable,you could specify a SizeGetter to calculate the message.
using MsgFoo2 = Protocol::MessageWrapper<0x02, Foo2, Foo2::SizeGetter>;
```


### Server
Based on asio,```rpc::Server``` has a nice speed.  
```c++
const char *const IP = "127.0.0.1";
unsigned short port = 8890;

// specify listen address.
Server server(IP, port);

// could only bind message callback by struct wrapped by rpc::Protocol::MessageWrapper,
// that's why you need to specfify message cmd when using rpc::Protocol::MessageWrapper.
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
    // this will block subsequent code.
    server.Start();
```


### Client
```c++
const char *const IP = "127.0.0.1";
unsigned short port = 8890;

// construct with server addredd.
Client client(IP, port);

// could only bind message callback by struct wrapped by rpc::Protocol::MessageWrapper,
// that's why you need to specfify message cmd when using rpc::Protocol::MessageWrapper.
client.Bind<MsgFoo2>([](const MsgFoo2 &req, Session &session) -> void 
{
    std::string_view sv(req->text, req->textSize);
    std::cout << sv << std::endl; 
});

// this will also block.So we use thread here.
std::thread([&]() -> void{ client.Start(); }).detach();

// WriteBuffer could be cast to any type wrapped by rpc::Protocol::MessageWrapper.
MsgFoo1 &msg = client.WriteBuffer();
msg->i = 1;
msg->d = 2;

// only class with Bytes() convert could be sent.
// all classes/struct wrapped by MessageWrapper has converter by default.
client.Send(msg);
```




## Full Usage
```c++
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
    std::thread([&]() -> void
                { server.Start(); })
        .detach();

    Client client(IP, port);
    client.Bind<MsgFoo2>([](const MsgFoo2 &req, Session &session) -> void
                         {
            std::string_view sv(req->text, req->textSize);
            std::cout << sv << std::endl; });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::thread([&]() -> void
                { client.Start(); })
        .detach();
    MsgFoo1 &msg = client.WriteBuffer();
    msg->i = 1;
    msg->d = 2;
    client.Send(msg);

    std::this_thread::sleep_for(std::chrono::seconds(100));
}
```

