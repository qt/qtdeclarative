/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**/
#ifndef QQMLDOMMOCK_P_H
#define QQMLDOMMOCK_P_H

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

#include "qqmldomitem_p.h"
#include "qqmldomconstants_p.h"
#include "qqmldomelements_p.h"
#include "qqmldomcomments_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>

#include <QtCore/QCborValue>
#include <QtCore/QCborMap>
#include <QtCore/QMutexLocker>
#include <QtCore/QPair>

#include <functional>
#include <limits>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

// mainly for debugging purposes
class MockObject final : public CommentableDomElement
{
public:
    constexpr static DomType kindValue = DomType::MockObject;
    DomType kind() const override { return kindValue; }

    MockObject(Path pathFromOwner = Path(), QMap<QString, MockObject> subObjects = {},
               QMap<QString, QCborValue> subValues = {})
        : CommentableDomElement(pathFromOwner), subObjects(subObjects), subValues(subValues)
    {
    }

    MockObject copy() const;
    std::pair<QString, MockObject> asStringPair() const;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;

    QMap<QString, MockObject> subObjects;
    QMap<QString, QCborValue> subValues;
};

// mainly for debugging purposes
class MockOwner final : public OwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(DomItem &self) const override;

public:
    constexpr static DomType kindValue = DomType::MockOwner;
    DomType kind() const override { return kindValue; }

    MockOwner(Path pathFromTop = Path(), int derivedFrom = 0,
              QMap<QString, MockObject> subObjects = {}, QMap<QString, QCborValue> subValues = {},
              QMap<QString, QMap<QString, MockObject>> subMaps = {},
              QMap<QString, QMultiMap<QString, MockObject>> subMultiMaps = {},
              QMap<QString, QList<MockObject>> subLists = {})
        : OwningItem(derivedFrom),
          pathFromTop(pathFromTop),
          subObjects(subObjects),
          subValues(subValues),
          subMaps(subMaps),
          subMultiMaps(subMultiMaps),
          subLists(subLists)
    {
    }

    MockOwner(Path pathFromTop, int derivedFrom, QDateTime dataRefreshedAt,
              QMap<QString, MockObject> subObjects = {}, QMap<QString, QCborValue> subValues = {})
        : OwningItem(derivedFrom, dataRefreshedAt),
          pathFromTop(pathFromTop),
          subObjects(subObjects),
          subValues(subValues)
    {
    }

    MockOwner(const MockOwner &o);

    std::shared_ptr<MockOwner> makeCopy(DomItem &self) const;
    Path canonicalPath(DomItem &self) const override;

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;

    Path pathFromTop;
    QMap<QString, MockObject> subObjects;
    QMap<QString, QCborValue> subValues;
    QMap<QString, QMap<QString, MockObject>> subMaps;
    QMap<QString, QMultiMap<QString, MockObject>> subMultiMaps;
    QMap<QString, QList<MockObject>> subLists;
};

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOMELEMENTS_P_H
