#ifndef _RPC_PROTOCOL_H_
#define _RPC_PROTOCOL_H_

#include <cstddef>
#include <memory>
#include "util.h"

namespace rpc
{
    template <typename T, typename ReturnSizeType = size_t>
    class DefaultMessageSizeGetter
    {
    public:
        ReturnSizeType operator()(const T &t) const { return staticSize_; }

    protected:
        static constexpr ReturnSizeType staticSize_ = sizeof(T);
    };

    template <typename SizeType, typename CmdType>
    class Protocol
    {
    public:
        using Size = SizeType;
        using Cmd = CmdType;
        static constexpr size_t sizeLength = sizeof(Size);
        static constexpr size_t cmdLength = sizeof(Cmd);
        static constexpr size_t headerLength = sizeLength + cmdLength;
        static constexpr Size maxSize = (~0);
        static constexpr Size bodySize = maxSize - headerLength;

        static bool IsSizeTypeUnsigned()
        {
            constexpr Size zero = 0;
            Size i = -1;
            return i > zero;
        }

        class Bytes
        {
        public:
            Bytes(const char *data, Protocol::Size size) : data_(data), size_(size) {}
            ~Bytes() {}

            inline char *Data() { return data_; }
            inline const char *Data() const { return data_; }
            inline Protocol::Size Size() { return size_; }
            inline const Protocol::Size Size() const { return size_; }

        protected:
            const char *data_;
            Protocol::Size size_;
        };

        template <typename Protocol::Cmd cmd, typename MessageType,
                  class MessageSizeGetter = DefaultMessageSizeGetter<MessageType, typename Protocol::Size>>
        class MessageWrapper
        {
        public:
            inline MessageType &Data() { return msg_; }
            inline const MessageType &Data() const { return msg_; }

            operator Bytes()
            {
                size_ = Protocol::headerLength + GetSize();
                cmd_ = staticCmdValue_;
                ASSERT(size_ <= Protocol::maxSize, "total size should smaller than maxSize");
                return Bytes(reinterpret_cast<const char *>(this), Size());
            }

            MessageType *operator->() { return &msg_; }
            const MessageType *operator->() const { return &msg_; }
            MessageType &operator*() { return msg_; }
            const MessageType &operator*() const { return msg_; }

        protected:
            typename Protocol::Size size_;
            typename Protocol::Cmd cmd_;
            MessageType msg_;

            static constexpr typename Protocol::Cmd staticCmdValue_ = cmd;
            static constexpr MessageSizeGetter messageSizeGetter_ = MessageSizeGetter();

            inline typename Protocol::Size GetSize() const
            {
                ASSERT(IsSizeTypeUnsigned(), "Protocol::SizeType should be an unsigned type");
                Protocol::Size size = messageSizeGetter_(msg_);
                ASSERT((size <= bodySize),
                       "message size should be smaller than Protocol::bodySize");
                return size;
            }

            inline typename Protocol::Size GetSize() { return const_cast<const MessageWrapper &>(*this).Size(); }
        };

        class Packet
        {
        public:
            template <typename T>
            operator const T() const = delete;
            template <typename T>
            operator T() = delete;

            template <typename T>
            operator const T &() const { return *reinterpret_cast<const T *>(this); }
            template <typename T>
            operator T &() { return *reinterpret_cast<T *>(this); }

            inline typename Protocol::Size Size()
            {
                ASSERT(IsSizeTypeUnsigned(), "Protocol::SizeType should be an unsigned type");
                return size_;
            }

            inline typename Protocol::Size Size() const { return const_cast<Packet &>(*this).Size(); }
            inline typename Protocol::Cmd Cmd() { return cmd_; }
            inline typename Protocol::Cmd Cmd() const { return const_cast<Packet &>(*this).Cmd(); }

        protected:
            typename Protocol::Size size_;
            typename Protocol::Cmd cmd_;
            char body_[Protocol::bodySize];
        };

        using WriteBuffer = Packet;
        using ReadBuffer = Packet;
    };
}

#endif