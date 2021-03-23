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
#include "qqmldommock_p.h"
#include "qqmldomitem_p.h"
#include "qqmldomcomments_p.h"

#include <QtCore/QBasicMutex>
#include <QtCore/QMutexLocker>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

MockObject MockObject::copy() const
{
    QMap<QString, MockObject> newObjs;
    auto objs = subObjects;
    auto itO = objs.cbegin();
    auto endO = objs.cend();
    while (itO != endO) {
        newObjs.insert(itO.key(), itO->copy());
        ++itO;
    }
    return MockObject(pathFromOwner(), newObjs, subValues);
}

std::pair<QString, MockObject> MockObject::asStringPair() const
{
    return std::make_pair(pathFromOwner().last().headName(), *this);
}

bool MockObject::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    static QHash<QString, QString> knownFields;
    static QBasicMutex m;
    auto toField = [](QString f) -> QStringView {
        QMutexLocker l(&m);
        if (!knownFields.contains(f))
            knownFields[f] = f;
        return knownFields[f];
    };
    bool cont = CommentableDomElement::iterateDirectSubpaths(self, visitor);
    auto itV = subValues.begin();
    auto endV = subValues.end();
    while (itV != endV) {
        cont = cont && self.dvValue(visitor, PathEls::Field(toField(itV.key())), *itV);
        ++itV;
    }
    auto itO = subObjects.begin();
    auto endO = subObjects.end();
    while (itO != endO) {
        cont = cont && self.dvItem(visitor, PathEls::Field(toField(itO.key())), [&self, &itO]() {
            return self.copy(&(*itO));
        });
        ++itO;
    }
    return cont;
}

std::shared_ptr<OwningItem> MockOwner::doCopy(DomItem &) const
{
    return std::shared_ptr<OwningItem>(new MockOwner(*this));
}

MockOwner::MockOwner(const MockOwner &o)
    : OwningItem(o), pathFromTop(o.pathFromTop), subValues(o.subValues)
{
    auto objs = o.subObjects;
    auto itO = objs.cbegin();
    auto endO = objs.cend();
    while (itO != endO) {
        subObjects.insert(itO.key(), itO->copy());
        ++itO;
    }
}

std::shared_ptr<MockOwner> MockOwner::makeCopy(DomItem &self) const
{
    return std::static_pointer_cast<MockOwner>(doCopy(self));
}

Path MockOwner::canonicalPath(DomItem &) const
{
    return pathFromTop;
}

bool MockOwner::iterateDirectSubpaths(DomItem &self, DirectVisitor visitor)
{
    static QHash<QString, QString> knownFields;
    static QBasicMutex m;
    auto toField = [](QString f) -> QStringView {
        QMutexLocker l(&m);
        if (!knownFields.contains(f))
            knownFields[f] = f;
        return knownFields[f];
    };
    {
        auto itV = subValues.begin();
        auto endV = subValues.end();
        while (itV != endV) {
            if (!self.dvValue(visitor, PathEls::Field(toField(itV.key())), *itV))
                return false;
            ++itV;
        }
    }
    {
        auto itO = subObjects.begin();
        auto endO = subObjects.end();
        while (itO != endO) {
            if (!self.dvItem(visitor, PathEls::Field(toField(itO.key())),
                             [&self, &itO]() { return self.copy(&(*itO)); }))
                return false;
            ++itO;
        }
    }
    {
        auto it = subMaps.begin();
        auto end = subMaps.end();
        while (it != end) {
            if (!self.dvWrapField(visitor, toField(it.key()), it.value()))
                return false;
            ++it;
        }
    }
    {
        auto it = subMultiMaps.begin();
        auto end = subMultiMaps.end();
        while (it != end) {
            if (!self.dvWrapField(visitor, toField(it.key()), it.value()))
                return false;
            ++it;
        }
    }
    {
        auto it = subLists.begin();
        auto end = subLists.end();
        while (it != end) {
            if (!self.dvWrapField(visitor, toField(it.key()), it.value()))
                return false;
            ++it;
        }
    }
    return true;
}

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
