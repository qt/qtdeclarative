// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


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

bool MockObject::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    bool cont = CommentableDomElement::iterateDirectSubpaths(self, visitor);
    auto itV = subValues.begin();
    auto endV = subValues.end();
    while (itV != endV) {
        cont = cont && self.invokeVisitorOnValue(visitor, PathEls::Key(itV.key()), *itV);
        ++itV;
    }
    auto itO = subObjects.begin();
    auto endO = subObjects.end();
    while (itO != endO) {
        cont = cont && visitor(PathEls::Key(itO.key()), [&self, &itO]() {
                   return self.copy(&(*itO));
               });
        ++itO;
    }
    return cont;
}

std::shared_ptr<OwningItem> MockOwner::doCopy(const DomItem &) const
{
    return std::make_shared<MockOwner>(*this);
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

std::shared_ptr<MockOwner> MockOwner::makeCopy(const DomItem &self) const
{
    return std::static_pointer_cast<MockOwner>(doCopy(self));
}

Path MockOwner::canonicalPath(const DomItem &) const
{
    return pathFromTop;
}

bool MockOwner::iterateDirectSubpaths(const DomItem &self, DirectVisitor visitor) const
{
    auto wrapAsKey = [&self, &visitor](const QString &name, auto &obj) {
        PathEls::PathComponent c = PathEls::Key(name);
        return visitor(c, [&self, &c, &obj]() { return self.wrap(c, obj); });
    };
    {
        auto itV = subValues.begin();
        auto endV = subValues.end();
        while (itV != endV) {
            if (!self.invokeVisitorOnValue(visitor, PathEls::Key(itV.key()), *itV))
                return false;
            ++itV;
        }
    }
    {
        auto itO = subObjects.begin();
        auto endO = subObjects.end();
        while (itO != endO) {
            if (!visitor(PathEls::Key(itO.key()),
                         [&self, &itO]() { return self.copy(&(*itO)); })) {
                return false;
            }
            ++itO;
        }
    }
    {
        auto it = subMaps.begin();
        auto end = subMaps.end();
        while (it != end) {
            if (!wrapAsKey(it.key(), it.value()))
                return false;
            ++it;
        }
    }
    {
        auto it = subMultiMaps.begin();
        auto end = subMultiMaps.end();
        while (it != end) {
            if (!wrapAsKey(it.key(), it.value()))
                return false;
            ++it;
        }
    }
    {
        auto it = subLists.begin();
        auto end = subLists.end();
        while (it != end) {
            if (!wrapAsKey(it.key(), it.value()))
                return false;
            ++it;
        }
    }
    return true;
}

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
