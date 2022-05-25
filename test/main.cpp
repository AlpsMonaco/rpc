#include <iostream>
#include <functional>
#include <string.h>
#include "include/rpc/protocol.h"

using Protocol = rpc::Protocol<uint32_t, uint32_t>;
// using Buffer = Protocol::Packet;

struct Foo1
{
    unsigned long long a;
    unsigned short b;
    char c;
};

struct Foo2
{
    unsigned short a;
    char c[30];

    struct SizeGetter
    {
        size_t operator()(const Foo2 &foo2) { return sizeof(foo2); }
    };

    operator int()
    {
        return *reinterpret_cast<int *>(c);
    }
};

void PrintFoo2(const Foo2 &foo2, const int &b)
{
    std::cout << foo2.a << " " << foo2.c << std::endl;
}

void PrintInt(const int &a, const int &b)
{
    std::cout << a << std::endl;
    std::cout << b << std::endl;
}

template <typename Message>
void WriteMessage(const Message &message)
{
}

void FooInt(const int &i)
{
    std::cout << i << std::endl;
}

void FooChar(const char &c)
{
    std::cout << c << std::endl;
}

void Foo(const char&c)
{
    FooInt(reinterpret_cast<const int &>(c));
}

int main(int argc, char **argv)
{
    using MsgFoo1 = Protocol::MessageWrapper<1, Foo1>;
    using MsgFoo2 = Protocol::MessageWrapper<2, Foo2, Foo2::SizeGetter>;

    MsgFoo1 foo1;

    // Buffer buffer;
    // MsgFoo1 &msgFoo1 = buffer;
    // msgFoo1->a = 1;
    // msgFoo1->b = 2;
    // msgFoo1->c = 3;
    // WriteBuffer(buffer);
}