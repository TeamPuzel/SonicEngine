// Created by Lua (TeamPuzel) on August 12th 2025.
// Copyright (c) 2025 All rights reserved.
//
// This header defines utilities for safely reloading object classes at runtime.
#pragma once
#include <rt>
#include <type_traits>

#if defined(_MSC_VER)
#define DLLEXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

// A type-safe and cross-platform object exporter.
// This is not for use in headers but rather the object entry point to expose it
// from the shared object file.
#define EXPORT_SONIC_OBJECT(CLASSNAME)                       \
static_assert(DynamicObject<CLASSNAME>::value);              \
extern "C" DLLEXPORT ObjectRebuilder __sonic_object_rebuild() {        \
    return (ObjectRebuilder) &CLASSNAME::rebuild;            \
}                                                            \
extern "C" DLLEXPORT ObjectSerializer __sonic_object_serialize() {     \
    return nullptr;                                          \
}                                                            \
extern "C" DLLEXPORT ObjectDeserializer __sonic_object_deserialize() { \
    return (ObjectDeserializer) &CLASSNAME::deserialize;     \
}
#define OBJECT_REBUILD "__sonic_object_rebuild"
#define OBJECT_SERIALIZE "__sonic_object_serialize"
#define OBJECT_DESERIALIZE "__sonic_object_deserialize"

namespace sonic {
    class Object;

    /// A game object loadable from files and hot-reloadable during gameplay.
    /// Obviously don't attempt rebuilding if the ABI was broken between reloads.
    ///
    /// trait SerializableObject {
    ///     static rebuild(Self const*);
    ///     serialize(BinaryWriter&) const;
    ///     static deserialize(BinaryReader&) -> Self;
    /// }
    template <typename, typename = void> struct DynamicObject : std::false_type {};
    template <typename Self> struct DynamicObject<Self, std::enable_if_t<
        std::is_same<decltype(Self::rebuild(std::declval<Self const&>())), Box<Object>>::value and
        std::is_same<decltype(std::declval<Self const&>().serialize(std::declval<rt::BinaryWriter&>())), void>::value and
        std::is_same<decltype(Self::deserialize(std::declval<rt::BinaryReader&>())), Box<Object>>::value
    >> : std::true_type {};

    using ObjectRebuilder    = auto (*) (Object const&) -> Box<Object>;
    using ObjectDeserializer = auto (*) (rt::BinaryReader&) -> Box<Object>;
    using ObjectSerializer   = auto (Object::*) (rt::BinaryWriter&) const -> void;
}
