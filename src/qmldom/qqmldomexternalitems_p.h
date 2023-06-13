// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLDOMEXTERNALITEMS_P_H
#define QQMLDOMEXTERNALITEMS_P_H

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
#include "qqmldommoduleindex_p.h"
#include "qqmldomcomments_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmldirparser_p.h>
#include <QtQmlCompiler/private/qqmljstyperesolver_p.h>
#include <QtCore/QMetaType>

#include <limits>
#include <memory>

Q_DECLARE_METATYPE(QQmlDirParser::Plugin)

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

/*!
\internal
\class QQmlJS::Dom::ExternalOwningItem

\brief A OwningItem that refers to an external resource (file,...)

Every owning item has a file or directory it refers to.


*/
class QMLDOM_EXPORT ExternalOwningItem: public OwningItem {
public:
    ExternalOwningItem(QString filePath, QDateTime lastDataUpdateAt, Path pathFromTop,
                       int derivedFrom = 0, QString code = QString());
    ExternalOwningItem(const ExternalOwningItem &o) = default;
    QString canonicalFilePath(DomItem &) const override;
    QString canonicalFilePath() const;
    Path canonicalPath(DomItem &) const override;
    Path canonicalPath() const;
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override
    {
        bool cont = OwningItem::iterateDirectSubpaths(self, visitor);
        cont = cont && self.dvValueLazyField(visitor, Fields::canonicalFilePath, [this]() {
            return canonicalFilePath();
        });
        cont = cont
                && self.dvValueLazyField(visitor, Fields::isValid, [this]() { return isValid(); });
        if (!code().isNull())
            cont = cont
                    && self.dvValueLazyField(visitor, Fields::code, [this]() { return code(); });
        return cont;
    }

    bool iterateSubOwners(DomItem &self, function_ref<bool(DomItem &owner)> visitor) override
    {
        bool cont = OwningItem::iterateSubOwners(self, visitor);
        cont = cont && self.field(Fields::components).visitKeys([visitor](QString, DomItem &comps) {
            return comps.visitIndexes([visitor](DomItem &comp) {
                return comp.field(Fields::objects).visitIndexes([visitor](DomItem &qmlObj) {
                    if (const QmlObject *qmlObjPtr = qmlObj.as<QmlObject>())
                        return qmlObjPtr->iterateSubOwners(qmlObj, visitor);
                    Q_ASSERT(false);
                    return true;
                });
            });
        });
        return cont;
    }

    bool isValid() const {
        QMutexLocker l(mutex());
        return m_isValid;
    }
    void setIsValid(bool val) {
        QMutexLocker l(mutex());
        m_isValid = val;
    }
    // null code means invalid
    const QString &code() const { return m_code; }

protected:
    QString m_canonicalFilePath;
    QString m_code;
    Path m_path;
    bool m_isValid = false;
};

class QMLDOM_EXPORT QmlDirectory final : public ExternalOwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(DomItem &) const override
    {
        return std::make_shared<QmlDirectory>(*this);
    }

public:
    constexpr static DomType kindValue = DomType::QmlDirectory;
    DomType kind() const override { return kindValue; }
    QmlDirectory(QString filePath = QString(), QStringList dirList = QStringList(),
                 QDateTime lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
                 int derivedFrom = 0);
    QmlDirectory(const QmlDirectory &o) = default;

    std::shared_ptr<QmlDirectory> makeCopy(DomItem &self) const
    {
        return std::static_pointer_cast<QmlDirectory>(doCopy(self));
    }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;

    const QMultiMap<QString, Export> &exports() const & { return m_exports; }

    const QMultiMap<QString, QString> &qmlFiles() const & { return m_qmlFiles; }

    bool addQmlFilePath(QString relativePath);

private:
    QMultiMap<QString, Export> m_exports;
    QMultiMap<QString, QString> m_qmlFiles;
};

class QMLDOM_EXPORT QmldirFile final : public ExternalOwningItem
{
    Q_DECLARE_TR_FUNCTIONS(QmldirFile)
protected:
    std::shared_ptr<OwningItem> doCopy(DomItem &) const override
    {
        auto copy = std::make_shared<QmldirFile>(*this);
        return copy;
    }

public:
    constexpr static DomType kindValue = DomType::QmldirFile;
    DomType kind() const override { return kindValue; }

    static ErrorGroups myParsingErrors();

    QmldirFile(QString filePath = QString(), QString code = QString(),
               QDateTime lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
               int derivedFrom = 0)
        : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::qmldirFilePath(filePath),
                             derivedFrom, code)
    {
    }
    QmldirFile(const QmldirFile &o) = default;

    static std::shared_ptr<QmldirFile> fromPathAndCode(QString path, QString code);

    std::shared_ptr<QmldirFile> makeCopy(DomItem &self) const
    {
        return std::static_pointer_cast<QmldirFile>(doCopy(self));
    }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;

    QmlUri uri() const { return m_uri; }

    const QSet<int> &majorVersions() const & { return m_majorVersions; }

    const QMultiMap<QString, Export> &exports() const & { return m_exports; }

    const QList<Import> &imports() const & { return m_imports; }

    const QList<Path> &qmltypesFilePaths() const & { return m_qmltypesFilePaths; }

    QMap<QString, QString> qmlFiles() const;

    bool designerSupported() const { return m_qmldir.designerSupported(); }

    QStringList classNames() const { return m_qmldir.classNames(); }

    QList<ModuleAutoExport> autoExports() const;
    void setAutoExports(const QList<ModuleAutoExport> &autoExport);

    void ensureInModuleIndex(DomItem &self, QString uri);

private:
    void parse();
    void setFromQmldir();

    QmlUri m_uri;
    QSet<int> m_majorVersions;
    QQmlDirParser m_qmldir;
    QList<QQmlDirParser::Plugin> m_plugins;
    QList<Import> m_imports;
    QList<ModuleAutoExport> m_autoExports;
    QMultiMap<QString, Export> m_exports;
    QList<Path> m_qmltypesFilePaths;
};

class QMLDOM_EXPORT JsFile final : public ExternalOwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(DomItem &) const override
    {
        auto copy = std::make_shared<JsFile>(*this);
        return copy;
    }

public:
    constexpr static DomType kindValue = DomType::JsFile;
    DomType kind() const override { return kindValue; }
    JsFile(QString filePath = QString(),
           QDateTime lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
           Path pathFromTop = Path(), int derivedFrom = 0)
        : ExternalOwningItem(filePath, lastDataUpdateAt, pathFromTop, derivedFrom)
    {
    }
    JsFile(const JsFile &o) = default;

    std::shared_ptr<JsFile> makeCopy(DomItem &self) const
    {
        return std::static_pointer_cast<JsFile>(doCopy(self));
    }

    std::shared_ptr<QQmlJS::Engine> engine() const { return m_engine; }
    JsResource rootComponent() const { return m_rootComponent; }

private:
    std::shared_ptr<QQmlJS::Engine> m_engine;
    JsResource m_rootComponent;
};

class QMLDOM_EXPORT QmlFile final : public ExternalOwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(DomItem &self) const override;

public:
    constexpr static DomType kindValue = DomType::QmlFile;
    DomType kind() const override { return kindValue; }

    QmlFile(const QmlFile &o);
    QmlFile(QString filePath = QString(), QString code = QString(),
            QDateTime lastDataUpdate = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
            int derivedFrom = 0);
    static ErrorGroups myParsingErrors();
    bool iterateDirectSubpaths(DomItem &self, DirectVisitor)
            override; // iterates the *direct* subpaths, returns false if a quick end was requested
    DomItem field(DomItem &self, QStringView name) const override
    {
        return const_cast<QmlFile *>(this)->field(self, name);
    }
    DomItem field(DomItem &self, QStringView name);
    std::shared_ptr<QmlFile> makeCopy(DomItem &self) const
    {
        return std::static_pointer_cast<QmlFile>(doCopy(self));
    }
    void addError(DomItem &self, ErrorMessage msg) override;

    const QMultiMap<QString, QmlComponent> &components() const & { return m_components; }
    void setComponents(const QMultiMap<QString, QmlComponent> &components)
    {
        m_components = components;
    }
    Path addComponent(const QmlComponent &component, AddOption option = AddOption::Overwrite,
                      QmlComponent **cPtr = nullptr)
    {
        QStringList nameEls = component.name().split(QChar::fromLatin1('.'));
        QString key = nameEls.mid(1).join(QChar::fromLatin1('.'));
        return insertUpdatableElementInMultiMap(Path::Field(Fields::components), m_components, key,
                                                component, option, cPtr);
    }

    void writeOut(DomItem &self, OutWriter &lw) const override;

    AST::UiProgram *ast() const
    {
        return m_ast; // avoid making it public? would make moving away from it easier
    }
    const QList<Import> &imports() const & { return m_imports; }
    void setImports(const QList<Import> &imports) { m_imports = imports; }
    Path addImport(const Import &i)
    {
        index_type idx = index_type(m_imports.size());
        m_imports.append(i);
        if (i.uri.isModule()) {
            m_importScope.addImport((i.importId.isEmpty()
                                             ? QStringList()
                                             : i.importId.split(QChar::fromLatin1('.'))),
                                    i.importedPath());
        } else {
            QString path = i.uri.absoluteLocalPath(canonicalFilePath());
            if (!path.isEmpty())
                m_importScope.addImport((i.importId.isEmpty()
                                                 ? QStringList()
                                                 : i.importId.split(QChar::fromLatin1('.'))),
                                        Paths::qmlDirPath(path));
        }
        return Path::Field(Fields::imports).index(idx);
    }
    std::shared_ptr<QQmlJS::Engine> engine() const { return m_engine; }
    RegionComments &comments() { return m_comments; }
    std::shared_ptr<AstComments> astComments() const { return m_astComments; }
    void setAstComments(std::shared_ptr<AstComments> comm) { m_astComments = comm; }
    FileLocations::Tree fileLocationsTree() const { return m_fileLocationsTree; }
    void setFileLocationsTree(FileLocations::Tree v) { m_fileLocationsTree = v; }
    const QList<Pragma> &pragmas() const & { return m_pragmas; }
    void setPragmas(QList<Pragma> pragmas) { m_pragmas = pragmas; }
    Path addPragma(const Pragma &pragma)
    {
        int idx = m_pragmas.size();
        m_pragmas.append(pragma);
        return Path::Field(Fields::pragmas).index(idx);
    }
    ImportScope &importScope() { return m_importScope; }
    const ImportScope &importScope() const { return m_importScope; }

    std::optional<std::shared_ptr<QQmlJSTypeResolver>> typeResolver() const
    {
        return m_typeResolver;
    }
    void setTypeResolver(const std::shared_ptr<QQmlJSTypeResolver> &typeResolver)
    {
        m_typeResolver = typeResolver;
    }

private:
    friend class QQmlDomAstCreator;
    std::shared_ptr<Engine> m_engine;
    AST::UiProgram *m_ast; // avoid? would make moving away from it easier
    std::shared_ptr<AstComments> m_astComments;
    RegionComments m_comments;
    FileLocations::Tree m_fileLocationsTree;
    QMultiMap<QString, QmlComponent> m_components;
    QList<Pragma> m_pragmas;
    QList<Import> m_imports;
    ImportScope m_importScope;
    std::optional<std::shared_ptr<QQmlJSTypeResolver>> m_typeResolver;
};

class QMLDOM_EXPORT QmltypesFile final : public ExternalOwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(DomItem &) const override
    {
        auto res = std::make_shared<QmltypesFile>(*this);
        return res;
    }

public:
    constexpr static DomType kindValue = DomType::QmltypesFile;
    DomType kind() const override { return kindValue; }

    QmltypesFile(QString filePath = QString(), QString code = QString(),
                 QDateTime lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
                 int derivedFrom = 0)
        : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::qmltypesFilePath(filePath),
                             derivedFrom, code)
    {
    }

    QmltypesFile(const QmltypesFile &o) = default;

    void ensureInModuleIndex(DomItem &self);

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor) override;
    std::shared_ptr<QmltypesFile> makeCopy(DomItem &self) const
    {
        return std::static_pointer_cast<QmltypesFile>(doCopy(self));
    }

    void addImport(const Import i)
    { // builder only: not threadsafe...
        m_imports.append(i);
    }
    const QList<Import> &imports() const & { return m_imports; }
    const QMultiMap<QString, QmltypesComponent> &components() const & { return m_components; }
    void setComponents(QMultiMap<QString, QmltypesComponent> c) { m_components = std::move(c); }
    Path addComponent(const QmltypesComponent &comp, AddOption option = AddOption::Overwrite,
                      QmltypesComponent **cPtr = nullptr)
    {
        for (const Export &e : comp.exports())
            addExport(e);
        return insertUpdatableElementInMultiMap(Path::Field(u"components"), m_components,
                                                comp.name(), comp, option, cPtr);
    }
    const QMultiMap<QString, Export> &exports() const & { return m_exports; }
    void setExports(QMultiMap<QString, Export> e) { m_exports = e; }
    Path addExport(const Export &e)
    {
        index_type i = m_exports.values(e.typeName).size();
        m_exports.insert(e.typeName, e);
        addUri(e.uri, e.version.majorVersion);
        return canonicalPath().field(Fields::exports).index(i);
    }

    const QMap<QString, QSet<int>> &uris() const & { return m_uris; }
    void addUri(QString uri, int majorVersion)
    {
        QSet<int> &v = m_uris[uri];
        if (!v.contains(majorVersion)) {
            v.insert(majorVersion);
        }
    }

private:
    QList<Import> m_imports;
    QMultiMap<QString, QmltypesComponent> m_components;
    QMultiMap<QString, Export> m_exports;
    QMap<QString, QSet<int>> m_uris;
};

class QMLDOM_EXPORT GlobalScope final : public ExternalOwningItem
{
protected:
    std::shared_ptr<OwningItem> doCopy(DomItem &) const override;

public:
    constexpr static DomType kindValue = DomType::GlobalScope;
    DomType kind() const override { return kindValue; }

    GlobalScope(QString filePath = QString(),
                QDateTime lastDataUpdateAt = QDateTime::fromMSecsSinceEpoch(0, QTimeZone::UTC),
                int derivedFrom = 0)
        : ExternalOwningItem(filePath, lastDataUpdateAt, Paths::globalScopePath(filePath),
                             derivedFrom)
    {
        setIsValid(true);
    }

    bool iterateDirectSubpaths(DomItem &self, DirectVisitor visitor) override;
    std::shared_ptr<GlobalScope> makeCopy(DomItem &self) const
    {
        return std::static_pointer_cast<GlobalScope>(doCopy(self));
    }
    QString name() const { return m_name; }
    Language language() const { return m_language; }
    GlobalComponent rootComponent() const { return m_rootComponent; }
    void setName(QString name) { m_name = name; }
    void setLanguage(Language language) { m_language = language; }
    void setRootComponent(const GlobalComponent &ob)
    {
        m_rootComponent = ob;
        m_rootComponent.updatePathFromOwner(Path::Field(Fields::rootComponent));
    }

private:
    QString m_name;
    Language m_language;
    GlobalComponent m_rootComponent;
};

} // end namespace Dom
} // end namespace QQmlJS
QT_END_NAMESPACE
#endif // QQMLDOMEXTERNALITEMS_P_H
