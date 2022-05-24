#ifndef _RPC_UTIL_H_
#define _RPC_UTIL_H_

#include <assert.h>
#include <stdio.h>

#define ASSERT(expression, comment)  \
    do                               \
    {                                \
        if (!(expression))           \
        {                            \
            printf("%s\n", comment); \
            assert(false);           \
        }                            \
                                     \
    } while (0)

namespace rpc
{

}

#endif