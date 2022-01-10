#pragma once
#include <core/common/common.hpp>
#include <core/types/types.hpp>
#include <core/filesystem/filesystem.hpp>

#include <iostream>

namespace legion::core::serialization
{
    struct serializer_view
    {
        serializer_view() = default;;
        virtual ~serializer_view() = default;

        template<typename Type>
        bool serialize(const std::string& name, Type&& value);
        bool serialize(const std::string& name, void* value, id_type typeId);

        virtual void serialize_int(const std::string& name, int serializable) LEGION_PURE;
        virtual void serialize_float(const std::string& name, float serializable) LEGION_PURE;
        virtual void serialize_double(const std::string& name, double serializable) LEGION_PURE;
        virtual void serialize_bool(const std::string& name, bool serializable) LEGION_PURE;
        virtual void serialize_string(const std::string& name, std::string_view serializable) LEGION_PURE;
        virtual void serialize_id_type(const std::string& name, id_type serializable) LEGION_PURE;

        template<typename Type>
        Type deserialize(std::string name);

        virtual common::result<int, fs_error> deserialize_int(const std::string& name) LEGION_PURE;
        virtual common::result<float, fs_error> deserialize_float(const std::string& name) LEGION_PURE;
        virtual common::result<double, fs_error> deserialize_double(const std::string& name) LEGION_PURE;
        virtual common::result<bool, fs_error> deserialize_bool(const std::string& name) LEGION_PURE;
        virtual common::result<std::string, fs_error> deserialize_string(const std::string& name) LEGION_PURE;
        virtual common::result<id_type, fs_error> deserialize_id_type(const std::string& name) LEGION_PURE;

        virtual void start_object() LEGION_PURE;
        virtual void start_object(std::string name) LEGION_PURE;
        virtual void end_object() LEGION_PURE;

        virtual void start_container(std::string name) LEGION_PURE;
        virtual void end_container() LEGION_PURE;

        virtual void start_read() LEGION_PURE;
        virtual void end_read() LEGION_PURE;

        virtual int start_read_array() LEGION_PURE;
        virtual void end_read_array() LEGION_PURE;

        virtual std::string get_key() LEGION_PURE;

        virtual common::result<void, fs_error> write(fs::view& file) LEGION_PURE;

        virtual common::result<void, fs_error> read(const fs::view& file) LEGION_PURE;
        virtual common::result<void, fs_error> read(const byte_vec& data) LEGION_PURE;
        virtual common::result<void, fs_error> read(byte_vec::iterator begin, byte_vec::iterator end) LEGION_PURE;
    };
}

#include <core/serialization/serializer_views/serializer_view.inl>

