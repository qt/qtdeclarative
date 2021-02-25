/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
#include "qqmldomtop_p.h"

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QRegularExpression>
#include <QtCore/QPair>
#include <QtCore/QCborArray>
#include <QtCore/QDebug>
#include <QtCore/QBasicMutex>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

using std::shared_ptr;


/*!
 \internal
 \brief QQml::Dom::DomTop::loadFile
 \param filePath
 the file path to load
 \param logicalPath
 the path from the
 \param callback
 a callback called with an canonical path, the old value, and the current value.
  \param loadOptions are
 if force is true the file is always read
 */

Path DomTop::pathFromOwner(const DomItem &) const
{
    return Path();
}

Path DomTop::canonicalPath(const DomItem &) const
{
    return canonicalPath();
}

DomItem DomTop::containingObject(const DomItem &) const
{
    return DomItem();
}

bool DomTop::iterateDirectSubpaths(DomItem &self, function<bool (Path, DomItem &)> visitor)
{
    bool cont = true;
    auto objs = m_extraOwningItems;
    auto itO = objs.cbegin();
    auto endO = objs.cend();
    while (itO != endO) {
        cont = cont && self.copy(*itO).toSubField(itO.key()).visit(visitor);
        ++itO;
    }
    return cont;
}

void DomTop::clearExtraOwningItems()
{
    QMutexLocker l(mutex());
    m_extraOwningItems.clear();
}

QMap<QString, std::shared_ptr<OwningItem> > DomTop::extraOwningItems() const
{
    QMutexLocker l(mutex());
    QMap<QString, std::shared_ptr<OwningItem> > res = m_extraOwningItems;
    return res;
}

void DomTop::setExtraOwningItem(QString fieldName, std::shared_ptr<OwningItem> item)
{
    QMutexLocker l(mutex());
    if (!item)
        m_extraOwningItems.remove(fieldName);
    else
        m_extraOwningItems.insert(fieldName, item);
}

/*!
\class QQmlJS::Dom::DomUniverse

\brief Represents a set of parsed/loaded modules libraries and a plugins

This can be used to share parsing and updates between several Dom models, and kickstart a model
without reparsing everything.

The universe is peculiar, because stepping into it from an environment looses the connection with
the environment.

This implementation is a placeholder, a later patch will introduce it.
 */

ErrorGroups DomUniverse::myErrors()
{
    static ErrorGroups groups = {{ DomItem::domErrorGroup, NewErrorGroup("Universe") }};
    return groups;
}

DomUniverse::DomUniverse(QString universeName, Options options):
    m_name(universeName), m_options(options)
{}

Path DomUniverse::canonicalPath() const
{
    return Path::root(u"universe");
}

bool DomUniverse::iterateDirectSubpaths(DomItem &self, function<bool (Path, DomItem &)> visitor)
{
    bool cont = true;
    cont = cont && DomTop::iterateDirectSubpaths(self, visitor);
    cont = cont && self.subDataField(Fields::name, name()).visit(visitor);
    cont = cont && self.subDataField(Fields::options, int(options())).visit(visitor);
    QQueue<ParsingTask> q = queue();
    cont = cont && self.subList(
                List(Path::field(Fields::queue),
                     [q](const DomItem &list, index_type i){
        if (i >= 0 && i < q.length())
            return list.subDataIndex(i, q.at(i).toCbor(), ConstantData::Options::FirstMapIsFields).item;
        else
            return DomItem();
    }, [q](const DomItem &){
        return index_type(q.length());
    }, nullptr, QLatin1String("ParsingTask"))
    ).visit(visitor);

    return cont;
}

std::shared_ptr<OwningItem> DomUniverse::doCopy(const DomItem &)
{
    QRegularExpression r(QRegularExpression::anchoredPattern(QLatin1String(R"(.*Copy([0-9]*)$)")));
    auto m = r.match(m_name);
    QString newName;
    if (m.hasMatch())
        newName = QStringLiteral(u"%1Copy%2").arg(m_name).arg(m.captured(1).toInt() + 1);
    else
        newName = m_name + QLatin1String("Copy");
    auto res = std::make_shared<DomUniverse>(newName);
    return res;
}

void DomUniverse::loadFile(const DomItem &self, QString filePath, QString logicalPath, Callback callback, LoadOptions loadOptions)
{
    loadFile(self, filePath, logicalPath, QString(), QDateTime::fromMSecsSinceEpoch(0), callback, loadOptions);
}

void DomUniverse::loadFile(const DomItem &self, QString canonicalFilePath, QString logicalPath, QString code, QDateTime codeDate, Callback callback, LoadOptions loadOptions)
{
    if (canonicalFilePath.endsWith(u".qml", Qt::CaseInsensitive) ||
        canonicalFilePath.endsWith(u".qmlannotation", Qt::CaseInsensitive) ||
        canonicalFilePath.endsWith(u".ui", Qt::CaseInsensitive)) {
        m_queue.enqueue(ParsingTask{
                                    QDateTime::currentDateTime(),
                                    loadOptions,
                                    DomType::QmlFile,
                                    canonicalFilePath,
                                    logicalPath,
                                    code,
                                    codeDate,
                                    self.ownerAs<DomUniverse>(),
                                    callback});
    } else if (canonicalFilePath.endsWith(u".qmltypes")) {
        m_queue.enqueue(ParsingTask{
                                    QDateTime::currentDateTime(),
                                    loadOptions,
                                    DomType::QmltypesFile,
                                    canonicalFilePath,
                                    logicalPath,
                                    code,
                                    codeDate,
                                    self.ownerAs<DomUniverse>(),
                                    callback});
    } else if (QStringView(u"qmldir").compare(QFileInfo(canonicalFilePath).fileName(), Qt::CaseInsensitive) == 0) {
        m_queue.enqueue(ParsingTask{
                                    QDateTime::currentDateTime(),
                                    loadOptions,
                                    DomType::QmldirFile,
                                    canonicalFilePath,
                                    logicalPath,
                                    code,
                                    codeDate,
                                    self.ownerAs<DomUniverse>(),
                                    callback});
    } else {
        self.addError(myErrors().error(tr("Ignoring request to load file of unknown type %1, calling callback immediately").arg(canonicalFilePath)).handle());
        Q_ASSERT(false && "loading non supported file type");
        callback(Path(), DomItem(), DomItem());
        return;
    }
    if (m_options & Option::SingleThreaded)
        execQueue(); // immediate execution in the same thread
}

template <typename T>
QPair<std::shared_ptr<ExternalItemPair<T>>,std::shared_ptr<ExternalItemPair<T>>> updateEntry(const DomItem &univ, std::shared_ptr<T> newItem, QMap<QString, std::shared_ptr<ExternalItemPair<T>>> &map, QBasicMutex *mutex)
{
    std::shared_ptr<ExternalItemPair<T>> oldValue;
    std::shared_ptr<ExternalItemPair<T>> newValue;
    QString canonicalPath = newItem->canonicalFilePath();
    QDateTime now = QDateTime::currentDateTime();
    {
        QMutexLocker l(mutex);
        auto it = map.find(canonicalPath);
        if (it != map.cend() && (*it) && (*it)->current) {
            oldValue = *it;
            QString oldCode = oldValue->current->code();
            QString newCode = newItem->code();
            if (!oldCode.isNull() && !newCode.isNull() && oldCode == newCode) {
                newValue = oldValue;
                if (newValue->current->lastDataUpdateAt() < newItem->lastDataUpdateAt())
                    newValue->current->refreshedDataAt(newItem->lastDataUpdateAt());
            } else if (oldValue->current->lastDataUpdateAt() > newItem->lastDataUpdateAt()) {
                newValue = oldValue;
            } else {
                newValue = oldValue->makeCopy(univ.copy(oldValue));
                newValue->current = newItem;
                newValue->currentExposedAt = now;
                if (newItem->isValid()) {
                    newValue->valid = newItem;
                    newValue->validExposedAt = now;
                }
                it = map.insert(it, canonicalPath, newValue);
            }
        } else {
            newValue = std::make_shared<ExternalItemPair<T>>
                (newItem->isValid() ? newItem : std::shared_ptr<T>(), newItem, now, now);
            map.insert(canonicalPath, newValue);
        }
    }
    return qMakePair(oldValue, newValue);
}

void DomUniverse::execQueue()
{
    ParsingTask t = m_queue.dequeue();
    shared_ptr<DomUniverse> topPtr = t.requestingUniverse.lock();
    if (!topPtr) {
        myErrors().error(tr("Ignoring callback for loading of %1: universe is not valid anymore").arg(t.canonicalPath)).handle();
    }
    Q_ASSERT(false  && "Unhandled kind in queue");
}

/*!
\class QQmlJS::Dom::DomEnvironment

\brief Represents a consistent set of types organized in modules, it is the top level of the DOM
 */

ErrorGroups DomEnvironment::myErrors() {
    static ErrorGroups res = {{NewErrorGroup("Dom")}};
    return res;
}


Path DomEnvironment::canonicalPath() const
{
    return Path::root(u"env");
}

bool DomEnvironment::iterateDirectSubpaths(DomItem &self, function<bool (Path, DomItem &)> visitor)
{
    bool cont = true;
    cont = cont && DomTop::iterateDirectSubpaths(self, visitor);
    DomItem univ = universe();
    cont = cont && visitor(Path::field(Fields::universe), univ);
    cont = cont && self.subDataField(Fields::options, int(options())).visit(visitor);
    DomItem baseItem = base();
    cont = cont && visitor(Path::field(Fields::base), baseItem);
    cont = cont && self.subList(List::fromQList<QString>(
                                    Path::field(Fields::loadPaths), loadPaths(),
                                    [](const DomItem &i, Path p, const QString &el){
        return i.subDataPath(p, el).item;
    })).visit(visitor);
    QQueue<Path> loadsWithWork;
    QQueue<Path> inProgress;
    int nAllLoadedCallbacks;
    {
        QMutexLocker l(mutex());
        loadsWithWork = m_loadsWithWork;
        inProgress = m_inProgress;
        nAllLoadedCallbacks = m_allLoadedCallback.length();
    }
    cont = cont && self.subList(
                List(Path::field(Fields::loadsWithWork),
                     [loadsWithWork](const DomItem &list, index_type i){
        if (i >= 0 && i < loadsWithWork.length())
            return list.subDataIndex(i, loadsWithWork.at(i).toString()).item;
        else
            return DomItem();
    }, [loadsWithWork](const DomItem &){
        return index_type(loadsWithWork.length());
    }, nullptr, QLatin1String("Path"))
    ).visit(visitor);
    cont = cont && self.subDataField(Fields::nAllLoadedCallbacks, nAllLoadedCallbacks).visit(visitor);
    return cont;
}

std::shared_ptr<OwningItem> DomEnvironment::doCopy(const DomItem &)
{
    shared_ptr<DomEnvironment> res;
    if (m_base)
        res = std::make_shared<DomEnvironment>(m_base, m_loadPaths, m_options);
    else
        res = std::make_shared<DomEnvironment>(m_universe, m_loadPaths, m_options);
    return res;
}

shared_ptr<DomUniverse> DomEnvironment::universe() const {
    if (m_universe)
        return m_universe;
    else if (m_base)
        return m_base->universe();
    else
        return {};
}

DomEnvironment::DomEnvironment(shared_ptr<DomUniverse> universe, QStringList loadPaths, Options options):
    m_options(options), m_universe(universe), m_loadPaths(loadPaths)
{}

DomEnvironment::DomEnvironment(shared_ptr<DomEnvironment> parent, QStringList loadPaths, Options options):
    m_options(options), m_base(parent), m_loadPaths(loadPaths)
{}

Path ExternalItemInfoBase::canonicalPath(const DomItem &self) const
{
    shared_ptr<ExternalOwningItem> current = currentItem();
    return current->canonicalPath(self.copy(current, current.get())).dropTail();
}

QString ExternalItemInfoBase::canonicalFilePath(const DomItem &self) const
{
    shared_ptr<ExternalOwningItem> current = currentItem();
    return current->canonicalFilePath(self.copy(current, current.get()));
}

Path ExternalItemInfoBase::pathFromOwner(const DomItem &self) const
{
    shared_ptr<ExternalOwningItem> current = currentItem();
    return current->pathFromOwner(self.copy(current, current.get())).dropTail();
}

bool ExternalItemInfoBase::iterateDirectSubpaths(DomItem &self, function<bool (Path, DomItem &)> visitor)
{
    if (!self.subDataField(Fields::currentRevision, currentRevision(self)).visit(visitor))
        return false;
    if (!self.subDataField(Fields::lastRevision, lastRevision(self)).visit(visitor))
        return false;
    if (!self.subDataField(Fields::lastValidRevision, QCborValue(lastValidRevision(self))).visit(visitor))
        return false;
    DomItem cItem = self.copy(currentItem(), currentItem().get());
    if (!visitor(Path::field(Fields::currentItem), cItem))
        return false;
    if (!self.subDataField(Fields::currentExposedAt, QCborValue(currentExposedAt())).visit(visitor))
        return false;
    return true;
}

int ExternalItemInfoBase::currentRevision(const DomItem &) const
{
    return currentItem()->revision();
}

int ExternalItemInfoBase::lastRevision(const DomItem &self) const
{
    Path p = currentItem()->canonicalPath();
    DomItem lastValue = self.universe()[p.mid(1, p.length() - 1)].field(u"revision");
    return static_cast<int>(lastValue.value().toInteger(0));
}

int ExternalItemInfoBase::lastValidRevision(const DomItem &self) const
{
    Path p = currentItem()->canonicalPath();
    DomItem lastValidValue = self.universe()[p.mid(1, p.length() - 2)].field(u"validItem").field(u"revision");
    return static_cast<int>(lastValidValue.value().toInteger(0));
}

QString ExternalItemPairBase::canonicalFilePath(const DomItem &self) const
{
    shared_ptr<ExternalOwningItem> current = currentItem();
    return current->canonicalFilePath(self.copy(current, current.get()));
}

Path ExternalItemPairBase::pathFromOwner(const DomItem &self) const
{
    shared_ptr<ExternalOwningItem> current = currentItem();
    return current->pathFromOwner(self.copy(current, current.get())).dropTail();
}

Path ExternalItemPairBase::canonicalPath(const DomItem &) const
{
    shared_ptr<ExternalOwningItem> current = currentItem();
    return current->canonicalPath().dropTail();
}

bool ExternalItemPairBase::iterateDirectSubpaths(DomItem &self, function<bool (Path, DomItem &)> visitor)
{
    if (!self.subDataField(Fields::currentIsValid, currentIsValid()).visit(visitor))
        return false;
    DomItem vItem = self.copy(validItem(), validItem().get());
    if (!visitor(Path::field(Fields::validItem), vItem))
        return false;
    DomItem cItem = self.copy(currentItem(), currentItem().get());
    if (!visitor(Path::field(Fields::currentItem), cItem))
        return false;
    if (!self.subDataField(Fields::validExposedAt, QCborValue(validExposedAt)).visit(visitor))
        return false;
    if (!self.subDataField(Fields::currentExposedAt, QCborValue(currentExposedAt)).visit(visitor))
        return false;
    return true;
}

bool ExternalItemPairBase::currentIsValid() const
{
    return currentItem() == validItem();
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE
