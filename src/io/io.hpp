// Created by Lua (TeamPuzel) on July 30th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include <primitive>
// #include <SDL3/SDL.h>

/// An abstract interface encapsulating all globally effecting operation.
class Io {
    protected:
    Io() = default;

    public:
    Io(Io const&) = delete;
    Io(Io&&) = delete;
    auto operator=(Io const&) -> Io& = delete;
    auto operator=(Io&&) -> Io& = delete;
    virtual ~Io() {}

    class Error {
        public:
        virtual ~Error() {}
    };

    class ClassLoader {
        public:
        virtual ~ClassLoader() {}
    };

    protected:
    template <typename R, typename E = void> class FutureState {
        enum { Unresolved, Result, Error, Cancelled } state { Unresolved };
        union {
            R result;
            E error;
        };

        void resolve(R&& r) {
            state = Result;
            result = std::move(r);
        }

        void fail(E&& e) {
            state = Error;
            error = std::move(e);
        }

        void cancel() {
            state = Cancelled;
        }

        public:
        using Res = R;
        using Err = E;

        class Promise final {
            FutureState* to;

            friend class Future;

            Promise(FutureState* to) : to(to) {}

            public:
            void resolve(R&& r) {
                to->resolve(std::move(r));
                to = nullptr;
            }

            void fail(E&& e) {
                to->fail(std::move(e));
                to = nullptr;
            }

            void cancel() {
                to->cancel();
                to = nullptr;
            }
        };

        virtual void run(Promise promise) = 0;

        struct AccessError final : public Error {
            enum : u8 {
                InvalidState,
                AwaitedCancelledFuture,
            } reason;
            AccessError(decltype(reason) reason) : reason(reason) {}
            AccessError invalid() { return InvalidState; }
            AccessError cancelled_await() { return AwaitedCancelledFuture; }
        };

        virtual ~FutureState() {
            switch (state) {
                case Result: result.~R(); break;
                case Error: error.~E(); break;
            }
            state = Cancelled;
        }

        auto await() -> R {
            switch (state) {
                case Unresolved: run(); break;
                case Cancelled: throw AccessError::cancelled_await();
                default: throw AccessError::invalid();
            }

            switch (state) {
                case Result: return std::move(result);
                case Error: throw std::move(error);
                default: throw AccessError::invalid();
            }
        }
    };

    public:
    template <typename R, typename E = void> class Future final {
        Box<FutureState<R, E>> state;

        public:
        auto await() -> R {
            return state->await();
        }
    };

    class File {
        public:
        class OpenError : public Error {};
    };

    virtual auto class_loader() -> ClassLoader& = 0;
    virtual auto open_file(std::string_view path) -> Future<File, File::OpenError> = 0;
};

/// An Io implementation in terms of SDL rather than C++ or libc primitives.
class SdlIo final : public Io {

};
