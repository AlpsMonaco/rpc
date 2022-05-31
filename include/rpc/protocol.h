#ifndef _RPC_PROTOCOL_H_
#define _RPC_PROTOCOL_H_

#include <cstddef>
#include <memory>
#include <map>
#include <functional>
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
            static Protocol::Cmd Cmd() { return cmd; }

            inline operator MessageType &() { return msg_; }
            inline operator const MessageType &() { return msg_; }

            operator Bytes()
            {
                size_ = Protocol::headerLength + GetSize();
                cmd_ = Cmd();
                ASSERT(size_ <= Protocol::maxSize, "total size should smaller than maxSize");
                return Bytes(reinterpret_cast<const char *>(this), size_);
            }

            MessageType *operator->() { return &msg_; }
            const MessageType *operator->() const { return &msg_; }
            MessageType &operator*() { return msg_; }
            const MessageType &operator*() const { return msg_; }

        protected:
            typename Protocol::Size size_;
            typename Protocol::Cmd cmd_;
            MessageType msg_;

            // static constexpr typename Protocol::Cmd staticCmdValue_ = cmd;
            static constexpr MessageSizeGetter messageSizeGetter_ = MessageSizeGetter();

            inline typename Protocol::Size GetSize() const
            {
                ASSERT(IsSizeTypeUnsigned(), "Protocol::SizeType should be an unsigned type");
                Protocol::Size size = messageSizeGetter_(msg_);
                ASSERT((size <= bodySize),
                       "message size should be smaller than Protocol::bodySize");
                return size;
            }

            inline typename Protocol::Size GetSize() { return const_cast<const MessageWrapper &>(*this).GetSize(); }
        };

        struct Packet
        {
            typename Protocol::Size size;
            typename Protocol::Cmd cmd;
            char data[Protocol::bodySize];

            static const Packet &Cast(const char *p)
            {
                return *reinterpret_cast<const Packet *>(p);
            }
        };

        template <typename SessionType>
        class MessageHandler
        {
        public:
            using Callback = std::function<void(const Packet &, SessionType &)>;
            using SharedPtr = std::shared_ptr<MessageHandler>;

            template <typename Message>
            struct Handler
            {
                using Type = std::function<void(const Message &, SessionType &)>;
            };

            MessageHandler() {}
            ~MessageHandler() {}

            inline Callback &operator[](const typename Protocol::Cmd cmd) { return callbacks_[cmd]; }

            template <typename Message>
            inline void Bind(const typename Handler<Message>::Type &handler)
            {
                callbacks_.emplace(Message::Cmd(), Wrap<Message>(handler));
            }

            template <typename Message>
            static Callback Wrap(const typename Handler<Message>::Type &f)
            {
                return [f](const Packet &packet, SessionType &session) -> void
                { f(*reinterpret_cast<const Message *>(&packet), session); };
            }

        protected:
            std::map<typename Protocol::Cmd, Callback> callbacks_;
        };

        class FlexibleBuffer
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

            char *Get() { return buffer_; }
            const char *Get() const { return buffer_; }

        protected:
            char buffer_[Protocol::maxSize];
        };

        using Buffer = FlexibleBuffer;
    };
}

#endif