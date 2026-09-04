// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitpropertypath_p.h"
#include "qqstylekitcontrolproperties_p.h"

QT_BEGIN_NAMESPACE

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

QT_END_NAMESPACE

#include "moc_qqstylekitpropertypath_p.cpp"
