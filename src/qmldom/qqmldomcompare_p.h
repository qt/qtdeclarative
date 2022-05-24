// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMLDOMCOMPARE_P_H
#define QMLDOMCOMPARE_P_H

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

#include "qqmldom_global.h"
#include "qqmldomitem_p.h"

#include <memory>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

bool domCompare(
        DomItem &i1, DomItem &i2, function_ref<bool(Path, DomItem &, DomItem &)> change,
        function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &)> filter = noFilter,
        Path p = Path());

enum DomCompareStrList { FirstDiff, AllDiffs };

QMLDOM_EXPORT QStringList domCompareStrList(
        DomItem &i1, DomItem &i2,
        function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &) const> filter = noFilter,
        DomCompareStrList stopAtFirstDiff = DomCompareStrList::FirstDiff);

inline QStringList domCompareStrList(
        MutableDomItem &i1, DomItem &i2,
        function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &) const> filter = noFilter,
        DomCompareStrList stopAtFirstDiff = DomCompareStrList::FirstDiff)
{
    DomItem ii1 = i1.item();
    return domCompareStrList(ii1, i2, filter, stopAtFirstDiff);
}

inline QStringList domCompareStrList(
        DomItem &i1, MutableDomItem &i2,
        function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &) const> filter = noFilter,
        DomCompareStrList stopAtFirstDiff = DomCompareStrList::FirstDiff)
{
    DomItem ii2 = i2.item();
    return domCompareStrList(i1, ii2, filter, stopAtFirstDiff);
}

inline QStringList domCompareStrList(
        MutableDomItem &i1, MutableDomItem &i2,
        function_ref<bool(DomItem &, const PathEls::PathComponent &, DomItem &) const> filter = noFilter,
        DomCompareStrList stopAtFirstDiff = DomCompareStrList::FirstDiff)
{
    DomItem ii1 = i1.item();
    DomItem ii2 = i2.item();
    return domCompareStrList(ii1, ii2, filter, stopAtFirstDiff);
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
#endif // QMLDOMCOMPARE_P_H
