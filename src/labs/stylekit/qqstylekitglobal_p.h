// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITGLOBAL_P_H
#define QQSTYLEKITGLOBAL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQml/QtQml>

QT_BEGIN_NAMESPACE

class QQSK: public QObject
{
    Q_OBJECT

public:
    enum class Delegate {
        NoDelegate                  = 0x0000,
        Control                     = 0x0001,
        Background                  = 0x0002,
        Handle                      = 0x0004,
        HandleFirst                 = 0x0008,
        HandleSecond                = 0x0010,
        Indicator                   = 0x0020,
        IndicatorForeground         = 0x0040,
        IndicatorFirst              = 0x0080,
        IndicatorFirstForeground    = 0x0100,
        IndicatorSecond             = 0x0200,
        IndicatorSecondForeground   = 0x0400,
        // Note: for now, "control.text" is not treated as a separate delegate, but
        // as an integrated part of "control". Hence, there is no "Text" value in this enum.
    };
    Q_DECLARE_FLAGS(Delegates, Delegate)
    Q_FLAG(Delegate)

    enum class PropertyGroup {
        Control,
        Background,
        Foreground,
        Border,
        Handle,
        Image,
        Indicator,
        Shadow,
        Text,
        PATH_ID_GROUP_COUNT,

        /* Sub types, like states, are a part of a propertys storage ID, not its Path ID.
         * They appear in the group path, but are handled differently. */
        DelegateSubtype0,
        DelegateSubtype1,
        DelegateSubtype2,

        /* Read options are not a part of either the Path ID nor the Storage ID. They
         * just offer a convenient API for providing read options when reading a property.
         * The Global flag is used to signal that a property should be read directly from
         * the global style, circumventing the local StyleKitReader cache. */
        GlobalFlag,

        Unspecified
    };
    Q_ENUM(PropertyGroup)

    enum class PropertyPathFlag : quint8 {
        NoFlags             = 0x0,
        DelegateSubtype0    = 0x1,
        DelegateSubtype1    = 0x2,
        DelegateSubtype2    = 0x4,
        Global              = 0x8
    };
    Q_DECLARE_FLAGS(PropertyPathFlags, PropertyPathFlag)
    Q_FLAG(PropertyPathFlag)

    enum class Property {
        NoProperty,
        BottomLeftRadius,
        BottomMargin,
        BottomPadding,
        BottomRightRadius,
        Clip,
        Color,
        Data,
        Delegate,
        FillMode,
        FillWidth,
        FillHeight,
        Gradient,
        HOffset,
        Image,
        LeftMargin,
        LeftPadding,
        Margins,
        MinimumWidth,
        MinimumHeight,
        Opacity,
        Padding,
        Radius,
        RightMargin,
        RightPadding,
        Rotation,
        Scale,
        Source,
        Spacing,
        TopLeftRadius,
        TopMargin,
        TopPadding,
        TopRightRadius,
        Transition,
        Variations,
        Visible,
        VOffset,
        Width,
        Height,
        Blur,
        Alignment,
        Bold,
        Italic,
        PointSize,
        COUNT
    };
    Q_ENUM(Property)

    enum class StateFlag {
        Unspecified = 0x000,
        Normal      = 0x001,
        Pressed     = 0x002,
        Hovered     = 0x004,
        Highlighted = 0x008,
        Focused     = 0x010,
        Checked     = 0x020,
        Vertical    = 0x040,
        Disabled    = 0x080,
        MAX_STATE   = 0x100,
    };
    Q_DECLARE_FLAGS(State, StateFlag)
    Q_FLAG(State)

    enum class Subclass {
        QQStyleKitState,
        QQStyleKitReader,
    };
    Q_ENUM(Subclass)

public:
    template <typename T, typename Owner, typename... Args>
    static inline T *lazyCreate(T *const &ptr, const Owner *self, Args&&... args)
    {
        if (!ptr) {
            auto *mutableSelf = const_cast<Owner *>(self);
            auto *&mutablePtr = const_cast<T *&>(ptr);
            mutablePtr = new T(std::forward<Args>(args)..., mutableSelf);
        }
        return ptr;
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QQSK::State)
Q_DECLARE_OPERATORS_FOR_FLAGS(QQSK::Delegates)
Q_DECLARE_OPERATORS_FOR_FLAGS(QQSK::PropertyPathFlags)

QT_END_NAMESPACE

#endif // QQSTYLEKITGLOBAL_P_H
