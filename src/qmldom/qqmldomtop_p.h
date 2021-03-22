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
#ifndef DOMTOP_H
#define DOMTOP_H

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

#include "qqmldomexternalitems_p.h"

#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QDateTime>

#include <QtCore/QCborValue>
#include <QtCore/QCborMap>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT ParsingTask {
    Q_GADGET
public:
    QCborMap toCbor() const {
        return QCborMap(
        {{ QString::fromUtf16(Fields::requestedAt), QCborValue(requestedAt)},
         { QString::fromUtf16(Fields::loadOptions), int(loadOptions)},
         { QString::fromUtf16(Fields::kind), int(kind)},
         { QString::fromUtf16(Fields::canonicalPath), canonicalPath},
         { QString::fromUtf16(Fields::logicalPath), logicalPath},
         { QString::fromUtf16(Fields::contents), contents},
         { QString::fromUtf16(Fields::contentsDate), QCborValue(contentsDate)},
         { QString::fromUtf16(Fields::hasCallback), bool(callback)}});
    }

    QDateTime requestedAt;
    LoadOptions loadOptions;
    DomType kind;
    QString canonicalPath;
    QString logicalPath;
    QString contents;
    QDateTime contentsDate;
    std::weak_ptr<DomUniverse> requestingUniverse; // make it a shared_ptr?
    function<void(Path, DomItem, DomItem)> callback;
};

class QMLDOM_EXPORT ExternalItemPairBase: public OwningItem { // all access should have the lock of the DomUniverse containing this
    Q_DECLARE_TR_FUNCTIONS(ExternalItemPairBase);
public:
    constexpr static DomType kindValue = DomType::ExternalItemPair;
    DomType kind() const override {  return kindValue; }
    ExternalItemPairBase(QDateTime validExposedAt = QDateTime::fromMSecsSinceEpoch(0),
                         QDateTime currentExposedAt = QDateTime::fromMSecsSinceEpoch(0),
                         int derivedFrom=0, QDateTime lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0)):
        OwningItem(derivedFrom, lastDataUpdateAt), validExposedAt(validExposedAt), currentExposedAt(currentExposedAt)
    {}
    ExternalItemPairBase(const ExternalItemPairBase &o):
        OwningItem(o), validExposedAt(o.validExposedAt), currentExposedAt(o.currentExposedAt)
    {}
    virtual std::shared_ptr<ExternalOwningItem> validItem() const = 0;
    virtual std::shared_ptr<ExternalOwningItem> currentItem() const = 0;

    QString canonicalFilePath(const DomItem &) const override;
    Path pathFromOwner(const DomItem &self) const override;
    Path canonicalPath(const DomItem &self) const override;
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) override;

    bool currentIsValid() const;

    std::shared_ptr<ExternalItemPairBase> makeCopy(const DomItem &self) {
        return std::static_pointer_cast<ExternalItemPairBase>(doCopy(self));
    }

    QDateTime lastDataUpdateAt() const override {
        if (currentItem())
            return currentItem()->lastDataUpdateAt();
        return ExternalItemPairBase::lastDataUpdateAt();
    }

    void refreshedDataAt(QDateTime tNew) override {
        return OwningItem::refreshedDataAt(tNew);
        if (currentItem())
            currentItem()->refreshedDataAt(tNew);
    }

    friend class DomUniverse;

    QDateTime validExposedAt;
    QDateTime currentExposedAt;
};

template<class T>
class QMLDOM_EXPORT ExternalItemPair: public ExternalItemPairBase { // all access should have the lock of the DomUniverse containing this
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override {
        return std::make_shared<ExternalItemPair>(*this);
    }

public:
    friend class DomUniverse;
    ExternalItemPair(std::shared_ptr<T> valid = {}, std::shared_ptr<T> current = {},
                     QDateTime validExposedAt = QDateTime::fromMSecsSinceEpoch(0),
                     QDateTime currentExposedAt = QDateTime::fromMSecsSinceEpoch(0),
                     int derivedFrom = 0, QDateTime lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0)):
        ExternalItemPairBase(validExposedAt, currentExposedAt, derivedFrom, lastDataUpdateAt),
        valid(valid), current(current)
    {}
    ExternalItemPair(const ExternalItemPair &o):
        ExternalItemPairBase(o), valid(o.valid), current(o.current)
    {
        QMutexLocker l(mutex());
    }
    std::shared_ptr<ExternalOwningItem> validItem() const override { return valid; }
    std::shared_ptr<ExternalOwningItem> currentItem() const override { return current; }
    std::shared_ptr<ExternalItemPair> makeCopy(const DomItem &self) {
        return std::static_pointer_cast<ExternalItemPair>(doCopy(self));
    }

    std::shared_ptr<T> valid;
    std::shared_ptr<T> current;
};

class QMLDOM_EXPORT DomTop: public OwningItem {
public:
    DomTop(QMap<QString, std::shared_ptr<OwningItem>> extraOwningItems = {}, int derivedFrom=0):
        OwningItem(derivedFrom), m_extraOwningItems(extraOwningItems)
    {}
    DomTop(const DomTop &o):
        OwningItem(o)
    {
        QMap<QString, std::shared_ptr<OwningItem>> items = o.extraOwningItems();
        {
            QMutexLocker l(mutex());
            m_extraOwningItems = items;
        }
    }
    using Callback = DomItem::Callback;

    virtual Path canonicalPath() const = 0;

    Path pathFromOwner(const DomItem &) const override;
    Path canonicalPath(const DomItem &) const override;
    DomItem containingObject(const DomItem&) const override;
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) override;

    void setExtraOwningItem(QString fieldName, std::shared_ptr<OwningItem> item);
    void clearExtraOwningItems();
    QMap<QString, std::shared_ptr<OwningItem>> extraOwningItems() const;
private:
    QMap<QString, std::shared_ptr<OwningItem>> m_extraOwningItems;
};

class QMLDOM_EXPORT DomUniverse: public DomTop {
    Q_DECLARE_TR_FUNCTIONS(DomUniverse);
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;
public:
    enum class Option{
        Default,
        SingleThreaded
    };
    Q_DECLARE_FLAGS(Options, Option);
    constexpr static DomType kindValue = DomType::DomUniverse;
    DomType kind() const override {  return kindValue; }

    static ErrorGroups myErrors();

    DomUniverse(QString universeName, Options options = Option::SingleThreaded);
    DomUniverse(const DomUniverse &) = delete;

    Path canonicalPath() const override;
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) override;
    std::shared_ptr<DomUniverse> makeCopy(const DomItem &self) {
        return std::static_pointer_cast<DomUniverse>(doCopy(self));
    }

    void loadFile(const DomItem &self, QString filePath, QString logicalPath,
                  Callback callback, LoadOptions loadOptions);
    void loadFile(const DomItem &self, QString canonicalFilePath, QString logicalPath,
                  QString code, QDateTime codeDate, Callback callback, LoadOptions loadOptions);
    void execQueue();

    QString name() const {
        return m_name;
    }
    Options options() const {
        return m_options;
    }
    QQueue<ParsingTask> queue() const {
        QMutexLocker l(mutex());
        return m_queue;
    }

private:
    QString m_name;
    Options m_options;
    QQueue<ParsingTask> m_queue;
};

    Q_DECLARE_OPERATORS_FOR_FLAGS(DomUniverse::Options)

class QMLDOM_EXPORT ExternalItemInfoBase: public OwningItem {
    Q_DECLARE_TR_FUNCTIONS(ExternalItemInfoBase);
public:
    constexpr static DomType kindValue = DomType::ExternalItemInfo;
    DomType kind() const override {  return kindValue; }
    ExternalItemInfoBase(QDateTime currentExposedAt = QDateTime::fromMSecsSinceEpoch(0),
                         int derivedFrom=0, QDateTime lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0)):
        OwningItem(derivedFrom, lastDataUpdateAt), m_currentExposedAt(currentExposedAt)
    {}
    ExternalItemInfoBase(const ExternalItemInfoBase &o):
        OwningItem(o), m_currentExposedAt(o.currentExposedAt()),
        m_logicalFilePaths(o.logicalFilePaths())
    {}

    virtual std::shared_ptr<ExternalOwningItem> currentItem() const = 0;

    QString canonicalFilePath(const DomItem &) const override;
    Path canonicalPath(const DomItem &) const override;
    Path pathFromOwner(const DomItem &self) const override;
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) override;

    int currentRevision(const DomItem &self) const;
    int lastRevision(const DomItem &self) const;
    int lastValidRevision(const DomItem &self) const;

    std::shared_ptr<ExternalItemInfoBase> makeCopy(const DomItem &self) {
        return std::static_pointer_cast<ExternalItemInfoBase>(doCopy(self));
    }

    QDateTime lastDataUpdateAt() const override {
        if (currentItem())
            return currentItem()->lastDataUpdateAt();
        return OwningItem::lastDataUpdateAt();
    }

    void refreshedDataAt(QDateTime tNew) override {
        return OwningItem::refreshedDataAt(tNew);
        if (currentItem())
            currentItem()->refreshedDataAt(tNew);
    }

    void ensureLogicalFilePath(QString path) {
        QMutexLocker l(mutex());
        if (!m_logicalFilePaths.contains(path))
            m_logicalFilePaths.append(path);
    }

    QDateTime currentExposedAt() const {
        QMutexLocker l(mutex()); // should not be needed, as it should not change...
        return m_currentExposedAt;
    }

    void setCurrentExposedAt(QDateTime d) {
        QMutexLocker l(mutex()); // should not be needed, as it should not change...
        m_currentExposedAt = d;
    }


    QStringList logicalFilePaths() const {
        QMutexLocker l(mutex());
        return m_logicalFilePaths;
    }

 private:
    friend class DomEnvironment;
    QDateTime m_currentExposedAt;
    QStringList m_logicalFilePaths;
};

template <typename T>
class ExternalItemInfo: public ExternalItemInfoBase {
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override {
        return std::make_shared<ExternalItemInfo>(*this);
    }
public:
    ExternalItemInfo(std::shared_ptr<T> current = std::shared_ptr<T>(),
                     QDateTime currentExposedAt = QDateTime::fromMSecsSinceEpoch(0),
                     int derivedFrom = 0, QDateTime lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0)):
        ExternalItemInfoBase(currentExposedAt, derivedFrom, lastDataUpdateAt), current(current)
    {}
    ExternalItemInfo(QString canonicalPath):
        current(std::make_shared<T>(canonicalPath))
    {}
    ExternalItemInfo(const ExternalItemInfo &o):
        ExternalItemInfoBase(o), current(o.current)
    {
        QMutexLocker l(mutex());
    }

    std::shared_ptr<ExternalItemInfo> makeCopy(const DomItem &self) {
        return std::static_pointer_cast<ExternalItemInfo>(doCopy(self));
    }

    std::shared_ptr<ExternalOwningItem> currentItem() const override {
        return current;
    }

    std::shared_ptr<T> current;
};

enum class EnvLookup { Normal, NoBase, BaseOnly };

enum class Changeable { ReadOnly, Writable };

class QMLDOM_EXPORT DomEnvironment: public DomTop
{
    Q_DECLARE_TR_FUNCTIONS(DomEnvironment);
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;
public:
    enum class Option {
        Default = 0x0,
        KeepValid = 0x1, // if there is a previous valid version, use that instead of the latest
        Exported = 0x2, // the current environment is accessible by multiple threads, one should only modify whole OwningItems, and in general load and do other operations in other (Child) environments
        NoReload = 0x4, // never reload something that was already loaded by the parent environment
        WeakLoad = 0x8, // load only the names of the available types, not the types (qml files) themselves
        SingleThreaded = 0x10, // do all operations in a single thread
        NoDependencies = 0x20 // will not load dependencies (useful when editing)
    };
    Q_DECLARE_FLAGS(Options, Option);

    static ErrorGroups myErrors();
    constexpr static DomType kindValue = DomType::DomEnvironment;
    DomType kind() const override {  return kindValue; }

    Path canonicalPath() const override;
    bool iterateDirectSubpaths(DomItem &self, function_ref<bool(Path, DomItem &)>) override;
    std::shared_ptr<DomEnvironment> makeCopy(const DomItem &self) {
        return std::static_pointer_cast<DomEnvironment>(doCopy(self));
    }

    std::shared_ptr<DomUniverse> universe() const;

    DomEnvironment(std::shared_ptr<DomUniverse> universe, QStringList loadPaths, Options options = Option::SingleThreaded);
    DomEnvironment(std::shared_ptr<DomEnvironment> parent, QStringList loadPaths, Options options = Option::SingleThreaded);
    DomEnvironment(const DomEnvironment &o) = delete;

    std::shared_ptr<ExternalItemInfo<QmlFile>> addQmlFile(std::shared_ptr<QmlFile> file);

    void commitToBase(const DomItem &self);

    Options options() const {
        return m_options;
    }

    std::shared_ptr<DomEnvironment> base() const {
        return m_base;
    }

    QStringList loadPaths() const {
        QMutexLocker l(mutex());
        return m_loadPaths;
    }

private:
    const Options m_options;
    const std::shared_ptr<DomEnvironment> m_base;
    const std::shared_ptr<DomUniverse> m_universe;
    QStringList m_loadPaths; // paths for qml
    bool m_singleThreadedLoadInProgress = false;
    QQueue<Path> m_loadsWithWork;
    QQueue<Path> m_inProgress;
    QList<Callback> m_allLoadedCallback;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(DomEnvironment::Options)

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // DOMTOP_H
