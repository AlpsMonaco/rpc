#include <iostream>
#include <map>
#include <string.h>
#include "include/rpc/protocol.h"

struct Foo
{
    unsigned long long bar1;
    char bar2[254];
};
std::map<int, int> m;

struct FooSizeGetter
{
    size_t operator()(const Foo &foo) const
    {
        static int i = 0;
        i++;
        std::cout << i << std::endl;
        return sizeof(foo.bar1) + strlen(foo.bar2) + 1;
    }
};

using protocol = rpc::Protocol<unsigned short, unsigned short>;

int main(int argc, char **argv)
{
    protocol::MessageWrapper<1, Foo, FooSizeGetter> msg;
    Foo &ffff = msg.Data();
    msg.Data().bar1 = 2;
    strcpy(ffff.bar2, "Hello");

    protocol::MessageWrapper<2, Foo, FooSizeGetter> msg2;
    memset(&msg2, 0, sizeof(msg2));
    msg2.Size();

    protocol::MessageWrapper<2, Foo, FooSizeGetter> msg3;
    memset(&msg3, 0, sizeof(msg2));
    msg3.Size();
}