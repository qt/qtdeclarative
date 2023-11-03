// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include "qqmldomelements_p.h"
#include "qqmldomexternalitems_p.h"

#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QDateTime>

#include <QtCore/QCborValue>
#include <QtCore/QCborMap>

#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

namespace QQmlJS {
namespace Dom {

class QMLDOM_EXPORT ParsingTask {
public:
    QCborMap toCbor() const {
        return QCborMap({ { QString::fromUtf16(Fields::requestedAt), QCborValue(requestedAt) },
                          { QString::fromUtf16(Fields::loadOptions), int(loadOptions) },
                          { QString::fromUtf16(Fields::kind), int(kind) },
                          { QString::fromUtf16(Fields::canonicalPath), file.canonicalPath() },
                          { QString::fromUtf16(Fields::logicalPath), file.logicalPath() },
                          { QString::fromUtf16(Fields::contents),
                            file.content() ? file.content()->data : QString() },
                          { QString::fromUtf16(Fields::contentsDate),
                            QCborValue(file.content() ? file.content()->date
                                                      : QDateTime::fromMSecsSinceEpoch(
                                                              0, QTimeZone::UTC)) },
                          { QString::fromUtf16(Fields::hasCallback), bool(callback) } });
    }

    QDateTime requestedAt;
    LoadOptions loadOptions;
    DomType kind;
    FileToLoad file;
    std::weak_ptr<DomUniverse> requestingUniverse; // make it a shared_ptr?
    function<void(Path, const DomItem &, const DomItem &)> callback;
};

class QMLDOM_EXPORT ExternalItemPairBase: public OwningItem { // all access should have the lock of the DomUniverse containing this
    Q_DECLARE_TR_FUNCTIONS(ExternalItemPairBase);
public:
    constexpr static DomType kindValue = DomType::ExternalItemPair;
    DomType kind() const final override { return kindValue; }
    ExternalItemPairBase(
            const QDateTime &validExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            const QDateTime &currentExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0,
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : OwningItem(derivedFrom, lastDataUpdateAt),
          validExposedAt(validExposedAt),
          currentExposedAt(currentExposedAt)
    {}
    ExternalItemPairBase(const ExternalItemPairBase &o):
        OwningItem(o), validExposedAt(o.validExposedAt), currentExposedAt(o.currentExposedAt)
    {}
    virtual std::shared_ptr<ExternalOwningItem> validItem() const = 0;
    virtual DomItem validItem(const DomItem &self) const = 0;
    virtual std::shared_ptr<ExternalOwningItem> currentItem() const = 0;
    virtual DomItem currentItem(const DomItem &self) const = 0;

    QString canonicalFilePath(const DomItem &) const final override;
    Path canonicalPath(const DomItem &self) const final override;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const final override;
    DomItem field(const DomItem &self, QStringView name) const final override
    {
        return OwningItem::field(self, name);
    }

    bool currentIsValid() const;

    std::shared_ptr<ExternalItemPairBase> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<ExternalItemPairBase>(doCopy(self));
    }

    QDateTime lastDataUpdateAt() const final override
    {
        if (currentItem())
            return currentItem()->lastDataUpdateAt();
        return ExternalItemPairBase::lastDataUpdateAt();
    }

    void refreshedDataAt(QDateTime tNew) final override
    {
        return OwningItem::refreshedDataAt(tNew);
        if (currentItem())
            currentItem()->refreshedDataAt(tNew);
    }

    friend class DomUniverse;

    QDateTime validExposedAt;
    QDateTime currentExposedAt;
};

template<class T>
class QMLDOM_EXPORT ExternalItemPair final : public ExternalItemPairBase
{ // all access should have the lock of the DomUniverse containing this
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        return std::make_shared<ExternalItemPair>(*this);
    }

public:
    constexpr static DomType kindValue = DomType::ExternalItemPair;
    friend class DomUniverse;
    ExternalItemPair(
            const std::shared_ptr<T> &valid = {}, const std::shared_ptr<T> &current = {},
            const QDateTime &validExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            const QDateTime &currentExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0,
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : ExternalItemPairBase(validExposedAt, currentExposedAt, derivedFrom, lastDataUpdateAt),
          valid(valid),
          current(current)
    {}
    ExternalItemPair(const ExternalItemPair &o):
        ExternalItemPairBase(o), valid(o.valid), current(o.current)
    {
    }
    std::shared_ptr<ExternalOwningItem> validItem() const override { return valid; }
    DomItem validItem(const DomItem &self) const override { return self.copy(valid); }
    std::shared_ptr<ExternalOwningItem> currentItem() const override { return current; }
    DomItem currentItem(const DomItem &self) const override { return self.copy(current); }
    std::shared_ptr<ExternalItemPair> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<ExternalItemPair>(doCopy(self));
    }

    std::shared_ptr<T> valid;
    std::shared_ptr<T> current;
};

class QMLDOM_EXPORT DomTop: public OwningItem {
public:
    DomTop(QMap<QString, OwnerT> extraOwningItems = {}, int derivedFrom = 0)
        : OwningItem(derivedFrom), m_extraOwningItems(extraOwningItems)
    {}
    DomTop(const DomTop &o):
        OwningItem(o)
    {
        QMap<QString, OwnerT> items = o.extraOwningItems();
        {
            QMutexLocker l(mutex());
            m_extraOwningItems = items;
        }
    }
    using Callback = DomItem::Callback;

    virtual Path canonicalPath() const = 0;

    Path canonicalPath(const DomItem &) const override;
    DomItem containingObject(const DomItem &) const override;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    template<typename T>
    void setExtraOwningItem(QString fieldName, std::shared_ptr<T> item)
    {
        QMutexLocker l(mutex());
        if (!item)
            m_extraOwningItems.remove(fieldName);
        else
            m_extraOwningItems.insert(fieldName, item);
    }

    void clearExtraOwningItems();
    QMap<QString, OwnerT> extraOwningItems() const;

private:
    QMap<QString, OwnerT> m_extraOwningItems;
};

class QMLDOM_EXPORT DomUniverse final : public DomTop
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(DomUniverse);
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;

public:
    enum class Option{
        Default,
        SingleThreaded
    };
    Q_ENUM(Option)
    Q_DECLARE_FLAGS(Options, Option);
    constexpr static DomType kindValue = DomType::DomUniverse;
    DomType kind() const override {  return kindValue; }

    static ErrorGroups myErrors();

    DomUniverse(QString universeName, Options options = Option::SingleThreaded);
    DomUniverse(const DomUniverse &) = delete;
    static std::shared_ptr<DomUniverse> guaranteeUniverse(std::shared_ptr<DomUniverse> univ);
    static DomItem create(QString universeName, Options options = Option::SingleThreaded);

    Path canonicalPath() const override;
    using DomTop::canonicalPath;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    std::shared_ptr<DomUniverse> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<DomUniverse>(doCopy(self));
    }

    void loadFile(const DomItem &self, const FileToLoad &file, Callback callback, LoadOptions loadOptions,
                  std::optional<DomType> fileType = std::optional<DomType>());
    void execQueue();

    void removePath(const QString &dir);

    std::shared_ptr<ExternalItemPair<GlobalScope>> globalScopeWithName(QString name) const
    {
        QMutexLocker l(mutex());
        return m_globalScopeWithName.value(name);
    }

    std::shared_ptr<ExternalItemPair<GlobalScope>> ensureGlobalScopeWithName(QString name)
    {
        if (auto current = globalScopeWithName(name))
            return current;
        auto newScope = std::make_shared<GlobalScope>(name);
        auto newValue = std::make_shared<ExternalItemPair<GlobalScope>>(
                newScope, newScope);
        QMutexLocker l(mutex());
        if (auto current = m_globalScopeWithName.value(name))
            return current;
        m_globalScopeWithName.insert(name, newValue);
        return newValue;
    }

    QSet<QString> globalScopeNames() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<GlobalScope>>> map;
        {
            QMutexLocker l(mutex());
            map = m_globalScopeWithName;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    std::shared_ptr<ExternalItemPair<QmlDirectory>> qmlDirectoryWithPath(QString path) const
    {
        QMutexLocker l(mutex());
        return m_qmlDirectoryWithPath.value(path);
    }
    QSet<QString> qmlDirectoryPaths() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<QmlDirectory>>> map;
        {
            QMutexLocker l(mutex());
            map = m_qmlDirectoryWithPath;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    std::shared_ptr<ExternalItemPair<QmldirFile>> qmldirFileWithPath(QString path) const
    {
        QMutexLocker l(mutex());
        return m_qmldirFileWithPath.value(path);
    }
    QSet<QString> qmldirFilePaths() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<QmldirFile>>> map;
        {
            QMutexLocker l(mutex());
            map = m_qmldirFileWithPath;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    std::shared_ptr<ExternalItemPair<QmlFile>> qmlFileWithPath(QString path) const
    {
        QMutexLocker l(mutex());
        return m_qmlFileWithPath.value(path);
    }
    QSet<QString> qmlFilePaths() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<QmlFile>>> map;
        {
            QMutexLocker l(mutex());
            map = m_qmlFileWithPath;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    std::shared_ptr<ExternalItemPair<JsFile>> jsFileWithPath(QString path) const
    {
        QMutexLocker l(mutex());
        return m_jsFileWithPath.value(path);
    }
    QSet<QString> jsFilePaths() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<JsFile>>> map;
        {
            QMutexLocker l(mutex());
            map = m_jsFileWithPath;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

    std::shared_ptr<ExternalItemPair<QmltypesFile>> qmltypesFileWithPath(QString path) const
    {
        QMutexLocker l(mutex());
        return m_qmltypesFileWithPath.value(path);
    }
    QSet<QString> qmltypesFilePaths() const
    {
        QMap<QString, std::shared_ptr<ExternalItemPair<QmltypesFile>>> map;
        {
            QMutexLocker l(mutex());
            map = m_qmltypesFileWithPath;
        }
        return QSet<QString>(map.keyBegin(), map.keyEnd());
    }

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
    QMap<QString, std::shared_ptr<ExternalItemPair<GlobalScope>>> m_globalScopeWithName;
    QMap<QString, std::shared_ptr<ExternalItemPair<QmlDirectory>>> m_qmlDirectoryWithPath;
    QMap<QString, std::shared_ptr<ExternalItemPair<QmldirFile>>> m_qmldirFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemPair<QmlFile>>> m_qmlFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemPair<JsFile>>> m_jsFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemPair<QmltypesFile>>> m_qmltypesFileWithPath;
    QQueue<ParsingTask> m_queue;
};

    Q_DECLARE_OPERATORS_FOR_FLAGS(DomUniverse::Options)

class QMLDOM_EXPORT ExternalItemInfoBase: public OwningItem {
    Q_DECLARE_TR_FUNCTIONS(ExternalItemInfoBase);
public:
    constexpr static DomType kindValue = DomType::ExternalItemInfo;
    DomType kind() const final override { return kindValue; }
    ExternalItemInfoBase(
            const Path &canonicalPath,
            const QDateTime &currentExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0,
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : OwningItem(derivedFrom, lastDataUpdateAt),
          m_canonicalPath(canonicalPath),
          m_currentExposedAt(currentExposedAt)
    {}
    ExternalItemInfoBase(const ExternalItemInfoBase &o) = default;

    virtual std::shared_ptr<ExternalOwningItem> currentItem() const = 0;
    virtual DomItem currentItem(const DomItem &) const = 0;

    QString canonicalFilePath(const DomItem &) const final override;
    Path canonicalPath() const { return m_canonicalPath; }
    Path canonicalPath(const DomItem &) const final override { return canonicalPath(); }
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const final override;
    DomItem field(const DomItem &self, QStringView name) const final override
    {
        return OwningItem::field(self, name);
    }

    int currentRevision(const DomItem &self) const;
    int lastRevision(const DomItem &self) const;
    int lastValidRevision(const DomItem &self) const;

    std::shared_ptr<ExternalItemInfoBase> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<ExternalItemInfoBase>(doCopy(self));
    }

    QDateTime lastDataUpdateAt() const final override
    {
        if (currentItem())
            return currentItem()->lastDataUpdateAt();
        return OwningItem::lastDataUpdateAt();
    }

    void refreshedDataAt(QDateTime tNew) final override
    {
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
    Path m_canonicalPath;
    QDateTime m_currentExposedAt;
    QStringList m_logicalFilePaths;
};

template<typename T>
class ExternalItemInfo final : public ExternalItemInfoBase
{
protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &) const override
    {
        return std::make_shared<ExternalItemInfo>(*this);
    }

public:
    constexpr static DomType kindValue = DomType::ExternalItemInfo;
    ExternalItemInfo(
            const std::shared_ptr<T> &current = std::shared_ptr<T>(),
            const QDateTime &currentExposedAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0,
            const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : ExternalItemInfoBase(current->canonicalPath().dropTail(), currentExposedAt, derivedFrom,
                               lastDataUpdateAt),
          current(current)
    {}
    ExternalItemInfo(QString canonicalPath) : current(new T(canonicalPath)) { }
    ExternalItemInfo(const ExternalItemInfo &o):
        ExternalItemInfoBase(o), current(o.current)
    {
    }

    std::shared_ptr<ExternalItemInfo> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<ExternalItemInfo>(doCopy(self));
    }

    std::shared_ptr<ExternalOwningItem> currentItem() const override {
        return current;
    }
    DomItem currentItem(const DomItem &self) const override { return self.copy(current); }

    std::shared_ptr<T> current;
};

class Dependency
{ // internal, should be cleaned, but nobody should use this...
public:
    bool operator==(Dependency const &o) const
    {
        return uri == o.uri && version.majorVersion == o.version.majorVersion
                && version.minorVersion == o.version.minorVersion && filePath == o.filePath;
    }
    QString uri; // either dotted uri or file:, http: https: uri
    Version version;
    QString filePath; // for file deps
    DomType fileType;
};

class QMLDOM_EXPORT LoadInfo final : public OwningItem
{
    Q_DECLARE_TR_FUNCTIONS(LoadInfo);

protected:
    std::shared_ptr<OwningItem> doCopy(const DomItem &self) const override;

public:
    constexpr static DomType kindValue = DomType::LoadInfo;
    DomType kind() const override { return kindValue; }

    enum class Status {
        NotStarted, // dependencies non checked yet
        Starting, // adding deps
        InProgress, // waiting for all deps to be loaded
        CallingCallbacks, // calling callbacks
        Done // fully loaded
    };

    LoadInfo(const Path &elPath = Path(), Status status = Status::NotStarted, int nLoaded = 0,
             int derivedFrom = 0,
             const QDateTime &lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC))
        : OwningItem(derivedFrom, lastDataUpdateAt),
          m_elementCanonicalPath(elPath),
          m_status(status),
          m_nLoaded(nLoaded)
    {
    }
    LoadInfo(const LoadInfo &o) : OwningItem(o), m_elementCanonicalPath(o.elementCanonicalPath())
    {
        {
            QMutexLocker l(o.mutex());
            m_status = o.m_status;
            m_nLoaded = o.m_nLoaded;
            m_toDo = o.m_toDo;
            m_inProgress = o.m_inProgress;
            m_endCallbacks = o.m_endCallbacks;
        }
    }

    Path canonicalPath(const DomItem &self) const override;

    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    std::shared_ptr<LoadInfo> makeCopy(const DomItem &self) const
    {
        return std::static_pointer_cast<LoadInfo>(doCopy(self));
    }
    void addError(const DomItem &self, ErrorMessage &&msg) override
    {
        self.path(elementCanonicalPath()).addError(std::move(msg));
    }

    void addEndCallback(const DomItem &self, std::function<void(Path, const DomItem &, const DomItem &)> callback);

    void advanceLoad(const DomItem &self);
    void finishedLoadingDep(const DomItem &self, const Dependency &d);
    void execEnd(const DomItem &self);

    Status status() const
    {
        QMutexLocker l(mutex());
        return m_status;
    }

    int nLoaded() const
    {
        QMutexLocker l(mutex());
        return m_nLoaded;
    }

    Path elementCanonicalPath() const
    {
        QMutexLocker l(mutex()); // we should never change this, remove lock?
        return m_elementCanonicalPath;
    }

    int nNotDone() const
    {
        QMutexLocker l(mutex());
        return m_toDo.size() + m_inProgress.size();
    }

    QList<Dependency> inProgress() const
    {
        QMutexLocker l(mutex());
        return m_inProgress;
    }

    QList<Dependency> toDo() const
    {
        QMutexLocker l(mutex());
        return m_toDo;
    }

    int nCallbacks() const
    {
        QMutexLocker l(mutex());
        return m_endCallbacks.size();
    }

private:
    void doAddDependencies(const DomItem &self);
    void addDependency(const DomItem &self, const Dependency &dep);

    Path m_elementCanonicalPath;
    Status m_status;
    int m_nLoaded;
    QQueue<Dependency> m_toDo;
    QList<Dependency> m_inProgress;
    QList<std::function<void(Path, const DomItem &, const DomItem &)>> m_endCallbacks;
};

enum class EnvLookup { Normal, NoBase, BaseOnly };

enum class Changeable { ReadOnly, Writable };

class QMLDOM_EXPORT RefCacheEntry
{
    Q_GADGET
public:
    enum class Cached { None, First, All };
    Q_ENUM(Cached)

    static RefCacheEntry forPath(const DomItem &el, Path canonicalPath);
    static bool addForPath(const DomItem &el, Path canonicalPath, const RefCacheEntry &entry,
                           AddOption addOption = AddOption::KeepExisting);

    Cached cached = Cached::None;
    QList<Path> canonicalPaths;
};

class QMLDOM_EXPORT DomEnvironment final : public DomTop
{
    Q_GADGET
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
    Q_ENUM(Option)
    Q_DECLARE_FLAGS(Options, Option);

    static ErrorGroups myErrors();
    constexpr static DomType kindValue = DomType::DomEnvironment;
    DomType kind() const override;

    Path canonicalPath() const override;
    using DomTop::canonicalPath;
    bool iterateDirectSubpaths(const DomItem &self, DirectVisitor) const override;
    DomItem field(const DomItem &self, QStringView name) const final override;

    std::shared_ptr<DomEnvironment> makeCopy(const DomItem &self) const;

    void loadFile(const DomItem &self, FileToLoad file, Callback loadCallback,
                  Callback directDepsCallback, Callback endCallback, LoadOptions loadOptions,
                  std::optional<DomType> fileType = std::optional<DomType>(),
                  ErrorHandler h = nullptr);
    void loadModuleDependency(const DomItem &self, QString uri, Version v,
                              Callback loadCallback = nullptr, Callback endCallback = nullptr,
                              ErrorHandler = nullptr);
    void loadBuiltins(const DomItem &self, Callback callback = nullptr, ErrorHandler h = nullptr);
    void removePath(const QString &path);

    std::shared_ptr<DomUniverse> universe() const;

    QSet<QString> moduleIndexUris(const DomItem &self, EnvLookup lookup = EnvLookup::Normal) const;
    QSet<int> moduleIndexMajorVersions(const DomItem &self, QString uri,
                                       EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ModuleIndex> moduleIndexWithUri(const DomItem &self, QString uri, int majorVersion,
                                                    EnvLookup lookup, Changeable changeable,
                                                    ErrorHandler errorHandler = nullptr);
    std::shared_ptr<ModuleIndex> moduleIndexWithUri(const DomItem &self, QString uri, int majorVersion,
                                                    EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<QmlDirectory>>
    qmlDirectoryWithPath(const DomItem &self, QString path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> qmlDirectoryPaths(const DomItem &self, EnvLookup options = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<QmldirFile>>
    qmldirFileWithPath(const DomItem &self, QString path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> qmldirFilePaths(const DomItem &self, EnvLookup options = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfoBase>
    qmlDirWithPath(const DomItem &self, QString path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> qmlDirPaths(const DomItem &self, EnvLookup options = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<QmlFile>>
    qmlFileWithPath(const DomItem &self, QString path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> qmlFilePaths(const DomItem &self, EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<JsFile>>
    jsFileWithPath(const DomItem &self, QString path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> jsFilePaths(const DomItem &self, EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<QmltypesFile>>
    qmltypesFileWithPath(const DomItem &self, QString path, EnvLookup options = EnvLookup::Normal) const;
    QSet<QString> qmltypesFilePaths(const DomItem &self, EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<GlobalScope>>
    globalScopeWithName(const DomItem &self, QString name, EnvLookup lookup = EnvLookup::Normal) const;
    std::shared_ptr<ExternalItemInfo<GlobalScope>>
    ensureGlobalScopeWithName(const DomItem &self, QString name, EnvLookup lookup = EnvLookup::Normal);
    QSet<QString> globalScopeNames(const DomItem &self, EnvLookup lookup = EnvLookup::Normal) const;

    explicit DomEnvironment(QStringList loadPaths, Options options = Option::SingleThreaded,
                            std::shared_ptr<DomUniverse> universe = nullptr);
    explicit DomEnvironment(std::shared_ptr<DomEnvironment> parent, QStringList loadPaths,
                            Options options = Option::SingleThreaded);
    DomEnvironment(const DomEnvironment &o) = delete;
    static DomItem create(QStringList loadPaths, Options options = Option::SingleThreaded,
                          const DomItem &universe = DomItem::empty);

    std::shared_ptr<ExternalItemInfo<QmlFile>>
    addQmlFile(std::shared_ptr<QmlFile> file, AddOption option = AddOption::KeepExisting);
    std::shared_ptr<ExternalItemInfo<QmlDirectory>>
    addQmlDirectory(std::shared_ptr<QmlDirectory> file, AddOption option = AddOption::KeepExisting);
    std::shared_ptr<ExternalItemInfo<QmldirFile>>
    addQmldirFile(std::shared_ptr<QmldirFile> file, AddOption option = AddOption::KeepExisting);
    std::shared_ptr<ExternalItemInfo<QmltypesFile>>
    addQmltypesFile(std::shared_ptr<QmltypesFile> file, AddOption option = AddOption::KeepExisting);
    std::shared_ptr<ExternalItemInfo<JsFile>> addJsFile(std::shared_ptr<JsFile> file,
                                                        AddOption option = AddOption::KeepExisting);
    std::shared_ptr<ExternalItemInfo<GlobalScope>>
    addGlobalScope(std::shared_ptr<GlobalScope> file, AddOption option = AddOption::KeepExisting);

    bool commitToBase(const DomItem &self, std::shared_ptr<DomEnvironment> validEnv = nullptr);

    void addLoadInfo(const DomItem &self, std::shared_ptr<LoadInfo> loadInfo);
    std::shared_ptr<LoadInfo> loadInfo(Path path) const;
    QList<Path> loadInfoPaths() const;
    QHash<Path, std::shared_ptr<LoadInfo>> loadInfos() const;
    void loadPendingDependencies(const DomItem &self);
    bool finishLoadingDependencies(const DomItem &self, int waitMSec = 30000);
    void addWorkForLoadInfo(Path elementCanonicalPath);

    Options options() const;

    std::shared_ptr<DomEnvironment> base() const;

    QStringList loadPaths() const;
    QStringList qmldirFiles() const;

    QString globalScopeName() const;

    static QList<Import> defaultImplicitImports();
    QList<Import> implicitImports() const;

    void addAllLoadedCallback(const DomItem &self, Callback c);

    void clearReferenceCache();
    void setLoadPaths(const QStringList &v);

private:
    friend class RefCacheEntry;
    template<typename T>
    QSet<QString> getStrings(function_ref<QSet<QString>()> getBase, const QMap<QString, T> &selfMap,
                             EnvLookup lookup) const;

    Callback callbackForQmlDirectory(const DomItem &self, Callback loadCallback,
                                     Callback directDepsCallback, Callback endCallback);
    Callback callbackForQmlFile(const DomItem &self, Callback loadCallback, Callback directDepsCallback,
                                Callback endCallback);
    Callback callbackForQmltypesFile(const DomItem &self, Callback loadCallback,
                                     Callback directDepsCallback, Callback endCallback);
    Callback callbackForQmldirFile(const DomItem &self, Callback loadCallback,
                                   Callback directDepsCallback, Callback endCallback);
    Callback callbackForJSFile(const DomItem &self, Callback loadCallback, Callback directDepsCallback,
                               Callback endCallback);


    std::shared_ptr<ModuleIndex> lookupModuleInEnv(const QString &uri, int majorVersion) const;
    // ModuleLookupResult contains the ModuleIndex pointer, and an indicator whether it was found
    // in m_base or in m_moduleIndexWithUri
    struct ModuleLookupResult {
        enum Origin :  bool {FromBase, FromGlobal};
        std::shared_ptr<ModuleIndex> module;
        Origin fromBase = FromGlobal;
    };
    // helper function used by the moduleIndexWithUri methods
    ModuleLookupResult moduleIndexWithUriHelper(const DomItem &self, QString uri, int majorVersion,
                                                    EnvLookup lookup = EnvLookup::Normal) const;

    const Options m_options;
    const std::shared_ptr<DomEnvironment> m_base;
    const std::shared_ptr<DomUniverse> m_universe;
    QStringList m_loadPaths; // paths for qml
    QString m_globalScopeName;
    QMap<QString, QMap<int, std::shared_ptr<ModuleIndex>>> m_moduleIndexWithUri;
    QMap<QString, std::shared_ptr<ExternalItemInfo<GlobalScope>>> m_globalScopeWithName;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmlDirectory>>> m_qmlDirectoryWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmldirFile>>> m_qmldirFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmlFile>>> m_qmlFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<JsFile>>> m_jsFileWithPath;
    QMap<QString, std::shared_ptr<ExternalItemInfo<QmltypesFile>>> m_qmltypesFileWithPath;
    QQueue<Path> m_loadsWithWork;
    QQueue<Path> m_inProgress;
    QHash<Path, std::shared_ptr<LoadInfo>> m_loadInfos;
    QList<Import> m_implicitImports;
    QList<Callback> m_allLoadedCallback;
    QHash<Path, RefCacheEntry> m_referenceCache;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(DomEnvironment::Options)

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // DOMTOP_H
