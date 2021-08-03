#pragma once
#include <atomic>
#include <mutex>
#include <memory>
#include <optional>
#include <thread>
#include <utility>

#include <core/platform/platform.hpp>
#include <core/types/types.hpp>
#include <core/async/wait_priority.hpp>
#include <core/async/spinlock.hpp>
#include <core/containers/delegate.hpp>
#include <core/common/result.hpp>
#include <core/common/exception.hpp>

namespace legion::core::async
{
    struct async_progress_base
    {
    protected:
        const size_type m_size;
        std::atomic<size_type> m_progress;

        template<typename T>
        constexpr static T precision_scale = static_cast<T>(1000);

        void complete_impl() noexcept;

    public:
        constexpr async_progress_base() noexcept : m_size(100 * precision_scale<size_type>), m_progress(0) {}
        constexpr explicit async_progress_base(float size) noexcept : m_size(static_cast<size_type>(size)* precision_scale<size_type>), m_progress(0) {}

        float size() const noexcept;
        size_type rawSize() const noexcept;
        size_type rawProgress() const noexcept;

        void advance_progress(float progress = 1.f) noexcept;
        bool is_done() const noexcept;
        float progress() const noexcept;
    };

    template<typename ReturnType>
    struct async_progress : public async_progress_base
    {
        using payload_type = ReturnType;
    protected:
        spinlock m_payloadLock;
        std::optional<payload_type> m_payload;

    public:
        constexpr async_progress() noexcept : async_progress_base() {}
        constexpr explicit async_progress(float size) noexcept : async_progress_base(size) {}

        void complete(ReturnType&& value) noexcept
        {
            std::lock_guard guard(m_payloadLock);
            m_payload = std::move(value);
            complete_impl();
        }

        common::result<payload_type> get_result()
        {
            std::lock_guard guard(m_payloadLock);
            if (m_payload)
                return std::move(*m_payload);
            return legion_exception_msg("Payload of async operation was not ready yet.");
        }
    };

    template<>
    struct async_progress<void> : public async_progress_base
    {
    public:
        constexpr async_progress() noexcept : async_progress_base() {}
        constexpr explicit async_progress(float size) noexcept : async_progress_base(size) {}

        void complete() noexcept { complete_impl(); }
    };

    struct async_operation_base
    {
    protected:
        std::shared_ptr<async_progress_base> m_progress;

    public:
        explicit async_operation_base(const std::shared_ptr<async_progress_base>& progress) noexcept;
        async_operation_base() noexcept = default;
        async_operation_base(const async_operation_base&) noexcept = default;
        async_operation_base(async_operation_base&&) noexcept = default;

        bool is_done() const noexcept;

        float progress() const noexcept;

        virtual void wait(wait_priority priority = wait_priority_normal) const noexcept;

        virtual ~async_operation_base() = default;
    };

    template<typename functor, typename payload>
    struct repeating_async_operation : public async_operation_base
    {
    protected:
        functor m_repeater;

    public:
        repeating_async_operation(const std::shared_ptr<async_progress<payload>>& progress, functor&& repeater) : async_operation_base(progress), m_repeater(repeater) {}
        repeating_async_operation() noexcept(std::is_nothrow_default_constructible_v<functor>) = default;
        repeating_async_operation(const repeating_async_operation&) noexcept(std::is_nothrow_copy_constructible_v<functor>) = default;
        repeating_async_operation(repeating_async_operation&&) noexcept(std::is_nothrow_move_constructible_v<functor>) = default;

        template<typename... argument_types>
        auto then(argument_types... args) const
        {
            wait();
            return m_repeater(std::forward<argument_types>(args)...);
        }

        template<typename... argument_types>
        auto then(wait_priority priority, argument_types... args) const
        {
            wait(priority);
            return m_repeater(std::forward<argument_types>(args)...);
        }

        payload&& get_result(wait_priority priority = wait_priority_normal)
        {
            wait(priority);
            static_cast<async_progress<payload>*>(m_progress.get())->get_result();
        }

        virtual ~repeating_async_operation() = default;
    };

#if !defined(DOXY_EXCLUDE)
    template<typename functor, typename payload>
    repeating_async_operation(const std::shared_ptr<async_progress<payload>>&, functor&&)->repeating_async_operation<functor, payload>;
#endif

    template<typename functor>
    struct repeating_async_operation<functor, void> : public async_operation_base
    {
    protected:
        functor m_repeater;

    public:
        repeating_async_operation(const std::shared_ptr<async_progress<void>>& progress, functor&& repeater) : async_operation_base(progress), m_repeater(repeater) {}
        repeating_async_operation() noexcept(std::is_nothrow_default_constructible_v<functor>) = default;
        repeating_async_operation(const repeating_async_operation&) noexcept(std::is_nothrow_copy_constructible_v<functor>) = default;
        repeating_async_operation(repeating_async_operation&&) noexcept(std::is_nothrow_move_constructible_v<functor>) = default;

        template<typename... argument_types>
        auto then(argument_types... args) const
        {
            wait();
            return m_repeater(std::forward<argument_types>(args)...);
        }

        template<typename... argument_types>
        auto then(wait_priority priority, argument_types... args) const
        {
            wait(priority);
            return m_repeater(std::forward<argument_types>(args)...);
        }

        virtual ~repeating_async_operation() = default;
    };

    template<typename payload>
    struct async_operation : public async_operation_base
    {
    public:
        async_operation(const std::shared_ptr<async_progress<payload>>& progress) : async_operation_base(progress) {}
        async_operation() noexcept = default;
        async_operation(const async_operation&) noexcept = default;
        async_operation(async_operation&&) noexcept = default;

        template<typename Functor, typename... argument_types>
        auto then(Functor&& func, argument_types... args) const
        {
            wait();
            return std::invoke(std::forward<Functor>(func), std::forward<argument_types>(args)...);
        }

        template<typename Functor, typename... argument_types>
        auto then(wait_priority priority, Functor&& func, argument_types... args) const
        {
            wait(priority);
            return std::invoke(std::forward<Functor>(func), std::forward<argument_types>(args)...);
        }

        payload&& get_result(wait_priority priority = wait_priority_normal)
        {
            wait(priority);
            static_cast<async_progress<payload>*>(m_progress.get())->get_result();
        }

        virtual ~async_operation() = default;
    };

#if !defined(DOXY_EXCLUDE)
    template<typename payload>
    async_operation(const std::shared_ptr<async_progress<payload>>&)->async_operation<payload>;
#endif

    template<>
    struct async_operation<void> : public async_operation_base
    {
    public:
        async_operation(const std::shared_ptr<async_progress<void>>& progress) : async_operation_base(progress) {}
        async_operation() noexcept = default;
        async_operation(const async_operation&) noexcept = default;
        async_operation(async_operation&&) noexcept = default;

        template<typename Functor, typename... argument_types>
        auto then(Functor&& func, argument_types... args) const
        {
            wait();
            return std::invoke(std::forward<Functor>(func), std::forward<argument_types>(args)...);
        }

        template<typename Functor, typename... argument_types>
        auto then(wait_priority priority, Functor&& func, argument_types... args) const
        {
            wait(priority);
            return std::invoke(std::forward<Functor>(func), std::forward<argument_types>(args)...);
        }

        virtual ~async_operation() = default;
    };
}
