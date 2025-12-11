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
        NoDelegate              = 0x0000,
        Control                 = 0x0001,
        Background              = 0x0002,
        Handle                  = 0x0004,
        HandleFirst             = 0x0008,
        HandleSecond            = 0x0010,
        Indicator               = 0x0020,
        IndicatorForeground     = 0x0040,
        IndicatorUp             = 0x0080,
        IndicatorUpForeground   = 0x0100,
        IndicatorDown           = 0x0200,
        IndicatorDownForeground = 0x0400,
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
        Gradient,
        HOffset,
        Image,
        ImplicitWidth,
        ImplicitHeight,
        LeftMargin,
        LeftPadding,
        Margins,
        MinimumWidth,
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

using PropertyPathId_t = quint32;
using PropertyStorageId = quint32;
using QQStyleKitExtendableControlType = quint32;
using QQStyleKitPropertyStorage = QHash<PropertyStorageId, QVariant>;

constexpr PropertyPathId_t maxPropertyStorageSpaceSize = std::numeric_limits<PropertyPathId_t>::max();
constexpr PropertyPathId_t maxStateCombinationCount = PropertyPathId_t(QQSK::StateFlag::MAX_STATE);
constexpr PropertyPathId_t stateStorageSpaceSize = maxPropertyStorageSpaceSize / maxStateCombinationCount;
constexpr PropertyPathId_t subtypeCount = PropertyPathId_t(QQSK::PropertyPathFlag::DelegateSubtype2) - PropertyPathId_t(QQSK::PropertyPathFlag::DelegateSubtype0) + 1;
constexpr PropertyPathId_t nestedGroupsStartSize = maxPropertyStorageSpaceSize / (maxStateCombinationCount * subtypeCount);
constexpr PropertyPathId_t subtypeStorageSpaceSize = maxPropertyStorageSpaceSize / (subtypeCount * maxStateCombinationCount);

struct QQStyleKitPropertyGroupSpace {
    PropertyPathId_t size = 0;
    PropertyPathId_t start = 0;
};

class PropertyPathId {
    Q_GADGET

public:
    enum class Flag {
        ExcludeSubtype,
        IncludeSubtype
    };
    Q_ENUM(Flag)

    PropertyPathId(
        const QQSK::Property property = QQSK::Property::NoProperty,
        const PropertyPathId_t groupStart = PropertyPathId_t(0),
        QQSK::PropertyGroup subtype = QQSK::PropertyGroup::DelegateSubtype0);

    inline QQSK::Property property() const { return m_property; }
    inline PropertyPathId_t pathId() const
    {
        /* The path ID is the property's identifier when its group path is taken
         * into account. Each property inside QQStyleKitControlProperties has a unique
         * path ID. For example, both 'background.color' and 'indicator.color' use the
         * same QQSK::Property (Color), but they still have different path IDs. */
        return m_groupStart + PropertyPathId_t(m_property);
    }
    inline PropertyStorageId storageId(QQSK::State state) const
    {
        /* To compute the fully qualified property ID used as a key in a storage map
         * (QMap) that holds its value, we need to prefix the path ID with the state ID,
         * since the same path can have different values in different states.
         * Because StateFlag::Normal == 1, we subtract 1 so that the address space for
         * properties in the Normal state starts at 0. */
        Q_ASSERT(state != QQSK::StateFlag::Unspecified);
        const PropertyPathId_t stateIndex = PropertyPathId_t(state) - 1;
        const PropertyPathId_t stateStart = stateIndex * stateStorageSpaceSize;
        return stateStart + pathId();
    }

private:
    QQSK::Property m_property;
    PropertyPathId_t m_groupStart;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITGLOBAL_P_H
