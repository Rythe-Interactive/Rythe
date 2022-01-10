#include <core/serialization/serializer.hpp>
#pragma once

namespace legion::core::serialization
{
    namespace detail
    {
        template<typename type>
        inline common::result<void, fs_error> serialize_container(type&& container, serializer_view& s_view, const std::string& name)
        {
            using container_type = typename remove_cvr_t<type>;
            using value_type = remove_cvr_t<typename container_type::value_type>;

            s_view.start_container(name);

            std::vector<std::string> warnings{};
            size_type i = 0;
            for (auto it = container.begin(); it != container.end(); ++it, i++)
            {
                auto _serializer = serializer_registry::get_serializer<value_type>();

                auto result = _serializer->serialize(*it, s_view, "item_" + std::to_string(i));
                warnings.insert(warnings.end(), result.warnings().begin(), result.warnings().end());

                if (result.has_error())
                {
                    s_view.end_container();
                    return { result.error(), warnings };
                }
            }

            s_view.end_container();

            return { common::success, warnings };
        }

        template<typename type>
        inline common::result<remove_cvr_t<type>, fs_error> deserialize_container(serializer_view& s_view)
        {
            using container_type = typename remove_cvr_t<type>;
            using value_type = remove_cvr_t<typename container_type::value_type>;

            size_type size = s_view.start_read_array();

            std::vector<value_type> tempContainer{};
            tempContainer.reserve(size);

            byte buffer[sizeof(value_type)];
            value_type* itemPtr = reinterpret_cast<value_type*>(buffer);
            std::vector<std::string> warnings{};

            for (size_type i = 0; i < size; i++)
            {
                auto _serializer = serializer_registry::get_serializer<value_type>();

                auto result = _serializer->deserialize(buffer, s_view, "item_" + std::to_string(i));
                warnings.insert(warnings.end(), result.warnings().begin(), result.warnings().end());

                if (result.has_error())
                {
                    s_view.end_read_array();
                    return { result.error(), warnings };
                }

                tempContainer.push_back(*itemPtr);
            }

            s_view.end_read_array();

            using iterator = typename container_type::iterator;
            using const_iterator = typename container_type::const_iterator;
            using input_iterator = typename std::vector<value_type>::iterator;

            container_type result{};

            if constexpr (has_insert_v<container_type, iterator(const_iterator, input_iterator, input_iterator)>)
                result.insert(result.begin(), tempContainer.begin(), tempContainer.end());
            else if constexpr (has_insert_v<container_type, void(input_iterator, input_iterator)>)
                result.insert(tempContainer.begin(), tempContainer.end());
            else if constexpr (has_size_v<container_type, size_type()> && has_at_v<container_type, value_type & (size_type)>)
            {
                for (size_type i = 0; i < result.size(); i++)
                    result[i] = tempContainer[i];
            }

            return { result, warnings };
        }

        inline common::result<void, fs_error> serialize_ent_data(const ecs::entity_data& ent_data, serializer_view& s_view, const std::string& name)
        {
            if (!ent_data.alive)
                return legion_fs_error("Entity does not exist.");

            std::vector<std::string> warnings{};

            s_view.start_object(name);

            s_view.serialize("name", ent_data.name);
            s_view.serialize("active", ent_data.active);
            auto result = detail::serialize_container(ent_data.children, s_view, "children");
            warnings.insert(warnings.end(), result.warnings().begin(), result.warnings().end());

            if (result.has_error())
            {
                s_view.end_object();
                return { result.error(), warnings };
            }

            s_view.start_container("components");

            auto ent = ecs::Registry::getEntity(ent_data.id);
            auto ent_composition = ent.component_composition();

            for (id_type typeId : ent_composition)
            {
                auto compName = std::string(type_hash::from_id(typeId).name());
                if (compName.empty())
                {
                    s_view.end_container();
                    s_view.end_object();
                    return { legion_fs_error("Component type " + std::to_string(typeId) + " has no type data."), warnings };
                }

                auto _serializer = serializer_registry::get_serializer(typeId);
                if (!_serializer)
                {
                    warnings.push_back("Could not find existing serializer for " + compName);
                    continue;
                }

                auto result = _serializer->serialize(ecs::Registry::getComponent(typeId, ent), s_view, compName);
                warnings.insert(warnings.end(), result.warnings().begin(), result.warnings().end());

                if (result.has_error())
                {
                    s_view.end_container();
                    s_view.end_object();
                    return { result.error(), warnings };
                }
            }

            s_view.end_container();
            s_view.end_object();
            return { common::success, warnings };
        }

        inline common::result<void, fs_error> deserialize_ent_data(void* target, serializer_view& s_view)
        {
            s_view.start_read();
            std::vector<std::string> warnings{};

            //Entity
            auto ent = ecs::Registry::createEntity();
            ent->name = s_view.deserialize<std::string>("name");
            ent->active = s_view.deserialize<bool>("active");
            ent->alive = s_view.deserialize<bool>("alive");

            //Children
            auto result = deserialize_container<ecs::entity_set>(s_view);
            warnings.insert(warnings.end(), result.warnings().begin(), result.warnings().end());

            if (result.has_error())
            {
                s_view.end_object();
                return { result.error(), warnings };
            }

            for (auto& child : result.value())
                ent.add_child(child);

            //Components
            size_type size = s_view.start_read_array();
            for (size_type i = 0; i < size; i++)
            {
                auto componentName = s_view.get_key();
                id_type typeId = nameHash(componentName);

                auto _serializer = serializer_registry::get_serializer(typeId);

                byte_vec buffer{};
                buffer.resize(_serializer->type_size());

                auto result = _serializer->deserialize(buffer.data(), s_view, componentName);
                warnings.insert(warnings.end(), result.warnings().begin(), result.warnings().end());

                if (result.has_error())
                {
                    s_view.end_object();
                    return { result.error(), warnings };
                }

                ecs::Registry::createComponent(typeId, ent, buffer.data());
            }

            s_view.end_read_array();

            *static_cast<ecs::entity*>(target) = ent;

            return { common::success, warnings };
        }
    }

    inline common::result<void, fs_error>  serializer<ecs::entity_data>::serialize(const void* ent, serializer_view& s_view, const std::string& name)
    {
        return detail::serialize_ent_data(*static_cast<const ecs::entity_data*>(ent), s_view, name);
    }

    inline common::result<void, fs_error> serializer<ecs::entity_data>::deserialize(void* target, serializer_view& view, L_MAYBEUNUSED const std::string& name)
    {
        return detail::deserialize_ent_data(target, view);
    }

    inline common::result<void, fs_error> serializer<ecs::entity>::serialize(const void* ent, serializer_view& s_view, const std::string& name)
    {
        return detail::serialize_ent_data(*static_cast<const ecs::entity*>(ent)->data, s_view, name);
    }

    inline common::result<void, fs_error> serializer<ecs::entity>::deserialize(void* target, serializer_view& view, L_MAYBEUNUSED const std::string& name)
    {
        ecs::entity_data data;
        auto result = detail::deserialize_ent_data(&data, view);
        *static_cast<ecs::entity*>(target) = ecs::Registry::getEntity(data.id);
        return result;
    }

    template<typename type>
    inline common::result<void, fs_error> serializer<type>::serialize(const void* serializable, serializer_view& s_view, const std::string& name)
    {
        using serializable_type = typename remove_cvr_t<type>;

        auto* ptr = static_cast<const serializable_type*>(serializable);

        if constexpr (is_serializable<serializable_type>::value)
        {
            s_view.serialize(name, *ptr);
        }
        else if constexpr (is_container<serializable_type>::value)
            detail::serialize_container(*ptr, s_view, name);
        else
        {
            reflector refl = make_reflector(*ptr);

            if (name.empty())
                s_view.start_object();
            else
                s_view.start_object(name);

            for (auto& var : refl.members)
            {
                if (var.is_object)
                {
                    auto _serializer = serializer_registry::get_serializer(var.object.typeId);
                    _serializer->serialize(var.object, s_view, var.name);
                }
                else
                {
                    auto _serializer = serializer_registry::get_serializer(var.primitive.typeId);
                    s_view.serialize(var.name, var.primitive.data, var.primitive.typeId);
                }
            }

            s_view.end_object();
        }
        return common::success;
    }

    template<typename type>
    inline common::result<void*, fs_error> serializer<type>::deserialize(serializer_view& s_view, std::string name)
    {
        using serializable_type = typename remove_cvr_t<type>;

        if constexpr (is_serializable<serializable_type>::value)
        {
            auto val = s_view.deserialize<serializable_type>(name);
            return &val;
        }
        else  if constexpr (is_container<serializable_type>::value)
        {
            auto container = detail::deserialize_container<serializable_type>(s_view).value();
            return &container;
        }
        else
        {
            auto reflector = make_reflector<serializable_type>(serializable_type());
            for_each(reflector,
                [&s_view](auto& name, auto& value)
                {
                    using value_type = typename remove_cvr_t<decltype(value)>;

                    if constexpr (is_serializable<value_type>::value)
                    {
                        value = s_view.deserialize<value_type>(name);
                    }
                    else  if constexpr (is_container<value_type>::value)
                        value = detail::deserialize_container<value_type>(s_view, name).value();
                    else
                    {
                        auto _serializer = serializer_registry::get_serializer<value_type>();
                        if (_serializer)
                        {
                            s_view.start_read();
                            value = *static_cast<value_type>(_serializer->deserialize(s_view, name));
                            s_view.end_read();
                        }
                        else
                            log::error("Serializer can't be created");
                    }
                });
            return &from_reflector(reflector);
        }
    }
}
