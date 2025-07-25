/*
 * Copyright (C) 2016 The Qt Company Ltd.
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#ifdef __cplusplus
#include <QtCore/qtconfigmacros.h>
#include <type_traits>
#endif

#ifdef __cplusplus
#ifdef QT_NAMESPACE
#define QT_NAMESPACE_REF ::QT_NAMESPACE::QtYoga
#else
#define QT_NAMESPACE_REF ::QtYoga
#endif

#define USE_QT_YOGA_NAMESPACE using namespace QT_NAMESPACE_REF;

#define QT_YOGA_NAMESPACE_BEGIN \
            QT_BEGIN_NAMESPACE \
            inline namespace QtYoga {
#define QT_YOGA_NAMESPACE_END \
            }                \
            QT_END_NAMESPACE
#endif

#ifdef __cplusplus
#define YG_EXTERN_C_BEGIN // extern "C" {
#define YG_EXTERN_C_END // }
#else
#define YG_EXTERN_C_BEGIN
#define YG_EXTERN_C_END
#endif

#if defined(__cplusplus)
#define YG_DEPRECATED(message) [[deprecated(message)]]
#elif defined(_MSC_VER)
#define YG_DEPRECATED(message) __declspec(deprecated(message))
#else
#define YG_DEPRECATED(message) __attribute__((deprecated(message)))
#endif

#ifdef _WINDLL
#define WIN_EXPORT // __declspec(dllexport)
#else
#define WIN_EXPORT
#endif

#ifndef YOGA_EXPORT
#ifdef _MSC_VER
#define YOGA_EXPORT
#else
#define YOGA_EXPORT // __attribute__((visibility("default")))
#endif
#endif

#ifdef NS_ENUM
// Cannot use NSInteger as NSInteger has a different size than int (which is the
// default type of a enum). Therefor when linking the Yoga C library into obj-c
// the header is a mismatch for the Yoga ABI.
#define YG_ENUM_BEGIN(name) NS_ENUM(int, name)
#define YG_ENUM_END(name)
#else
#define YG_ENUM_BEGIN(name) enum name
#define YG_ENUM_END(name) name
#endif

#ifdef __cplusplus
#define YG_DEFINE_ENUM_FLAG_OPERATORS(name)                       \
  extern "C++" {                                                  \
  constexpr inline name operator~(name a) {                       \
    return static_cast<name>(                                     \
        ~static_cast<std::underlying_type<name>::type>(a));       \
  }                                                               \
  constexpr inline name operator|(name a, name b) {               \
    return static_cast<name>(                                     \
        static_cast<std::underlying_type<name>::type>(a) |        \
        static_cast<std::underlying_type<name>::type>(b));        \
  }                                                               \
  constexpr inline name operator&(name a, name b) {               \
    return static_cast<name>(                                     \
        static_cast<std::underlying_type<name>::type>(a) &        \
        static_cast<std::underlying_type<name>::type>(b));        \
  }                                                               \
  constexpr inline name operator^(name a, name b) {               \
    return static_cast<name>(                                     \
        static_cast<std::underlying_type<name>::type>(a) ^        \
        static_cast<std::underlying_type<name>::type>(b));        \
  }                                                               \
  inline name& operator|=(name& a, name b) {                      \
    return reinterpret_cast<name&>(                               \
        reinterpret_cast<std::underlying_type<name>::type&>(a) |= \
        static_cast<std::underlying_type<name>::type>(b));        \
  }                                                               \
  inline name& operator&=(name& a, name b) {                      \
    return reinterpret_cast<name&>(                               \
        reinterpret_cast<std::underlying_type<name>::type&>(a) &= \
        static_cast<std::underlying_type<name>::type>(b));        \
  }                                                               \
  inline name& operator^=(name& a, name b) {                      \
    return reinterpret_cast<name&>(                               \
        reinterpret_cast<std::underlying_type<name>::type&>(a) ^= \
        static_cast<std::underlying_type<name>::type>(b));        \
  }                                                               \
  }
#else
#define YG_DEFINE_ENUM_FLAG_OPERATORS(name)
#endif

#ifdef __cplusplus
QT_YOGA_NAMESPACE_BEGIN
namespace facebook {
namespace yoga {
namespace enums {

template <typename T>
constexpr int count(); // can't use `= delete` due to a defect in clang < 3.9

namespace detail {
template <int... xs>
constexpr int n() {
  return sizeof...(xs);
}
} // namespace detail

} // namespace enums
} // namespace yoga
} // namespace facebook
QT_YOGA_NAMESPACE_END
#endif

#define YG_ENUM_DECL(NAME, ...)                               \
  typedef YG_ENUM_BEGIN(NAME){__VA_ARGS__} YG_ENUM_END(NAME); \
  WIN_EXPORT const char* NAME##ToString(NAME);

#ifdef __cplusplus
#define YG_ENUM_SEQ_DECL(NAME, ...)  \
  YG_ENUM_DECL(NAME, __VA_ARGS__)    \
  QT_YOGA_NAMESPACE_BEGIN            \
  YG_EXTERN_C_END                    \
  namespace facebook {               \
  namespace yoga {                   \
  namespace enums {                  \
  template <>                        \
  constexpr int count<NAME>() {      \
    return detail::n<__VA_ARGS__>(); \
  }                                  \
  }                                  \
  }                                  \
  }                                  \
  YG_EXTERN_C_BEGIN                  \
  QT_YOGA_NAMESPACE_END
#else
#define YG_ENUM_SEQ_DECL YG_ENUM_DECL
#endif
