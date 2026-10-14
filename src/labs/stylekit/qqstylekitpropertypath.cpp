// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitpropertypath_p.h"
#include "qqstylekitcontrolproperties_p.h"
#include "qqstylekitdebug_p.h"

QT_BEGIN_NAMESPACE

QHash<PropertyPathId, QString> QQStyleKitPropertyPath::s_cachedPathStrings;

QQStyleKitPropertyPath::QQStyleKitPropertyPath(
    const QQStyleKitPropertyGroup *group, QQSK::Property property, Flag flag)
    : m_property(property)
{
    PropertyPathId subtypeIndex = 0;
    if (flag == Flag::IncludeSubtype) {
        if (group->pathFlags() & QQSK::PropertyPathFlag::DelegateSubtype1)
            subtypeIndex = 1;
        else if (group->pathFlags() & QQSK::PropertyPathFlag::DelegateSubtype2)
            subtypeIndex = 2;
    }

    const PropertyPathId subtypeStart = subtypeIndex * subtypeStorageSpaceSize;
    const PropertyPathId groupStart = group->groupSpace().start;
    m_groupStart = subtypeStart + groupStart;
}

QString QQStyleKitPropertyPath::pathIdToPathString(PropertyPathId pathId)
{
    if (s_cachedPathStrings.contains(pathId))
        return s_cachedPathStrings.value(pathId);

    constexpr PropertyPathId rootGroupsSize = nestedGroupsStartSize / nestedGroupCount;
    const auto groupMetaEnum = QMetaEnum::fromType<QQSK::PropertyGroup>();
    const auto propertyMetaEnum = QMetaEnum::fromType<QQSK::Property>();

    const QQSK::PropertyGroup subType = extractSubtype(pathId);
    PropertyPathId nestedGroupStart = pathId % subtypeStorageSpaceSize;
    PropertyPathId nestedGroupSize = rootGroupsSize;
    PropertyPathId nestedGroupIndex = nestedGroupStart / nestedGroupSize;
    auto groupType = QQSK::PropertyGroup(nestedGroupIndex);

    QString pathString;

    if (groupType != QQSK::PropertyGroup::Control) {
        QString groupName = QString::fromLatin1(groupMetaEnum.valueToKey(static_cast<int>(groupType)));
        groupName[0] = groupName[0].toLower();
        pathString = groupName;

        switch (subType) {
        case QQSK::PropertyGroup::DelegateSubtype1:
            pathString += ".first"_L1;
            break;
        case QQSK::PropertyGroup::DelegateSubtype2:
            pathString += ".second"_L1;
            break;
        default:
            break;
        }

        while (true) {
            nestedGroupStart -= nestedGroupIndex * nestedGroupSize;
            nestedGroupSize /= nestedGroupCount;
            nestedGroupIndex = nestedGroupStart / nestedGroupSize;
            groupType = QQSK::PropertyGroup(nestedGroupIndex);
            if (groupType == QQSK::PropertyGroup::Control)
                break;

            QString nestedName = QString::fromLatin1(groupMetaEnum.valueToKey(static_cast<int>(groupType)));
            nestedName[0] = nestedName[0].toLower();
            pathString += '.'_L1 + nestedName;
        }
    }

    QQSK::Property property = QQSK::Property(nestedGroupStart);
    if (property != QQSK::Property::NoProperty) {
        QString propertyName = QString::fromLatin1(propertyMetaEnum.valueToKey(static_cast<int>(property)));
        propertyName[0] = propertyName[0].toLower();
        if (!pathString.isEmpty())
            pathString += '.'_L1;
        pathString += propertyName;
    }

    s_cachedPathStrings.insert(pathId, pathString);
    return pathString;
}

QString QQStyleKitPropertyPath::storageIdToPathString(PropertyStorageId storageId)
{
    const QString pathStr = pathIdToPathString(extractPropertyPathId(storageId));
    const QString stateStr = QQStyleKitDebug::stateToString(extractState(storageId));
    return stateStr + '.'_L1 + pathStr;
}

QT_END_NAMESPACE

#include "moc_qqstylekitpropertypath_p.cpp"
