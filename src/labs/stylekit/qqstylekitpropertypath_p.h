// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITPROPERTYPATH_P_H
#define QQSTYLEKITPROPERTYPATH_P_H

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

#include "qqstylekitglobal_p.h"

/* Each style property in StyleKit needs a unique PropertyStorageId that can be used as
 * a key in the map that stores its value. To compute such an ID, we must consider the
 * property's full nested path, since properties like 'background.color' and
 * 'background.border.color' refer to different values.
 *
 * Because a property may have multiple values depending on the control's state and
 * subtype, we distinguish between a property's path ID and its storage ID. The path
 * ID represents the portion of the property path that does not vary during lookups.
 * For example, in the full path:
 *
 *     "button.pressed.indicator.up.background.color"
 *
 * the portion that is invariant is:
 *
 *     "indicator.background.color"
 *
 * The other parts of the path—such as the control type ('button'), the state ('pressed'),
 * and the subtype ('up')—are resolved dynamically by the propagation engine. During lookup,
 * the engine substitutes these components in decreasing order of specificity. For instance:
 *
 * - If the property is not found on 'button', it falls back to 'abstractButton'.
 * - If it is not found in the 'pressed' state, it falls back to 'normal'.
 * - If it is not found in the 'up' subtype, it falls back to 'indicator'.
 *
 * These varying components are prepended in sequence by the propagation engine to form
 * the final PropertyStorageId, which uniquely identifies the stored value in the map.
 *
 * Note that a property path may also include groups known as Options. These are not part
 * of the Path ID or the Storage ID; they are simply flags used by QQStyleKitPropertyResolver
 * to control how a property should be read.
 *
 * In general, the structure of a property path is:
 *
 *     control.options.states.subtype.nested_group_path.property
 *
 * However, for API convenience, subtypes are written inside the delegate they belong to.
 * For example, although the storage path is "spinBox.up.indicator.background.color", the
 * style syntax is "spinBox.indicator.up.background.color". */

QT_BEGIN_NAMESPACE

class QQStyleKitPropertyGroup;

using PropertyPathId = quint32;
using PropertyStorageId = quint32;
using QQStyleKitExtendableControlType = quint32;
using QQStyleKitPropertyStorage = QHash<PropertyStorageId, QVariant>;

constexpr PropertyPathId maxPropertyStorageSpaceSize = std::numeric_limits<PropertyPathId>::max();
constexpr PropertyPathId nestedGroupCount = PropertyPathId(QQSK::PropertyGroup::PATH_ID_GROUP_COUNT);
constexpr PropertyPathId maxStateCombinationCount = PropertyPathId(QQSK::StateFlag::MAX_STATE);
constexpr PropertyPathId stateStorageSpaceSize = maxPropertyStorageSpaceSize / maxStateCombinationCount;
constexpr PropertyPathId subtypeCount = PropertyPathId(QQSK::PropertyPathFlag::DelegateSubtype2) - PropertyPathId(QQSK::PropertyPathFlag::DelegateSubtype0) + 1;
constexpr PropertyPathId nestedGroupsStartSize = maxPropertyStorageSpaceSize / (maxStateCombinationCount * subtypeCount);
constexpr PropertyPathId subtypeStorageSpaceSize = maxPropertyStorageSpaceSize / (subtypeCount * maxStateCombinationCount);

struct QQStyleKitPropertyGroupSpace {
    PropertyPathId size = 0;
    PropertyPathId start = 0;
};

class QQStyleKitPropertyPath {
    Q_GADGET

public:
    enum class Flag {
        ExcludeSubtype,
        IncludeSubtype
    };
    Q_ENUM(Flag)

    QQStyleKitPropertyPath() : m_property(QQSK::Property::NoProperty), m_groupStart(0) {}
    QQStyleKitPropertyPath(
        const QQStyleKitPropertyGroup *group,
        QQSK::Property property,
        Flag flag = Flag::IncludeSubtype);

    inline QQSK::Property property() const { return m_property; }

    inline PropertyPathId pathId() const
    {
        /* The path ID is the property's identifier when its group path is taken
         * into account. Each property inside QQStyleKitControlProperties has a unique
         * path ID. For example, both 'background.color' and 'indicator.color' use the
         * same QQSK::Property (Color), but they still have different path IDs. */
        return m_groupStart + PropertyPathId(m_property);
    }

    inline PropertyStorageId createStorageId(QQSK::State state) const
    {
        /* To compute the fully qualified property ID used as a key in a storage map
         * (QMap) that holds its value, we need to prefix the path ID with the state ID,
         * since the same path can have different values in different states.
         * Because StateFlag::Normal == 1, we subtract 1 so that the address space for
         * properties in the Normal state starts at 0. */
        Q_ASSERT(state != QQSK::StateFlag::Unspecified);
        const PropertyPathId stateIndex = PropertyPathId(state) - 1;
        const PropertyPathId stateStart = stateIndex * stateStorageSpaceSize;
        return stateStart + pathId();
    }

    static inline QQSK::State extractState(PropertyStorageId storageId)
    {
        /* Reverse the storageId function above, and return the state from the
         * given PropertyStorageId. The state occupies the most significant portion
         * of storageId. Adding 1 reverses the -1 applied in storageId() when encoding
         * the state. */
        const PropertyPathId stateIndex = storageId / stateStorageSpaceSize;
        return QQSK::State(stateIndex + 1);
    }

    static inline PropertyPathId extractPropertyPathId(PropertyStorageId storageId,
        QQStyleKitPropertyPath::Flag flag = QQStyleKitPropertyPath::Flag::IncludeSubtype)
    {
        /* Reverse the storageId function above, and return the path ID from the
         * given PropertyStorageId. The state is stripped by taking the remainder,
         * leaving the path ID including the subtype. When ExcludeSubtype, the subtype
         * slab is stripped with a second remainder operation. */
        if (flag == Flag::IncludeSubtype)
            return storageId % stateStorageSpaceSize;
        else
            return (storageId % stateStorageSpaceSize) % subtypeStorageSpaceSize;
    }

private:
    QQSK::Property m_property;
    PropertyPathId m_groupStart;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITPROPERTYPATH_P_H
