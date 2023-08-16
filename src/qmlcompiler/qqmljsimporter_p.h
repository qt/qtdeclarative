// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSIMPORTER_P_H
#define QQMLJSIMPORTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qtqmlcompilerexports_p.h>

#include "qqmljscontextualtypes_p.h"
#include "qqmljsscope_p.h"
#include "qqmljsresourcefilemapper_p.h"
#include <QtQml/private/qqmldirparser_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
class Import
{
public:
    Import() = default;
    Import(QString prefix, QString name, QTypeRevision version, bool isFile, bool isDependency);

    bool isValid() const;

    QString prefix() const { return m_prefix; }
    QString name() const { return m_name; }
    QTypeRevision version() const { return m_version; }
    bool isFile() const { return m_isFile; }
    bool isDependency() const { return m_isDependency; }

private:
    QString m_prefix;
    QString m_name;
    QTypeRevision m_version;
    bool m_isFile = false;
    bool m_isDependency = false;

    friend inline size_t qHash(const Import &key, size_t seed = 0) noexcept
    {
        return qHashMulti(seed, key.m_prefix, key.m_name, key.m_version,
                          key.m_isFile, key.m_isDependency);
    }

    friend inline bool operator==(const Import &a, const Import &b)
    {
        return a.m_prefix == b.m_prefix && a.m_name == b.m_name && a.m_version == b.m_version
                && a.m_isFile == b.m_isFile && a.m_isDependency == b.m_isDependency;
    }
};
}

class QQmlJSImportVisitor;
class QQmlJSLogger;
class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSImporter
{
public:
    using ImportedTypes = QQmlJS::ContextualTypes;

    QQmlJSImporter(const QStringList &importPaths, QQmlJSResourceFileMapper *mapper,
                   bool useOptionalImports = false);

    QQmlJSResourceFileMapper *resourceFileMapper() const { return m_mapper; }
    void setResourceFileMapper(QQmlJSResourceFileMapper *mapper) { m_mapper = mapper; }

    QQmlJSResourceFileMapper *metaDataMapper() const { return m_metaDataMapper; }
    void setMetaDataMapper(QQmlJSResourceFileMapper *mapper) { m_metaDataMapper = mapper; }

    ImportedTypes importBuiltins();
    void importQmldirs(const QStringList &qmltypesFiles);

    QQmlJSScope::Ptr importFile(const QString &file);
    ImportedTypes importDirectory(const QString &directory, const QString &prefix = QString());

    // ### qmltc needs this. once re-written, we no longer need to expose this
    QHash<QString, QQmlJSScope::Ptr> importedFiles() const { return m_importedFiles; }

    ImportedTypes importModule(const QString &module, const QString &prefix = QString(),
                               QTypeRevision version = QTypeRevision(),
                               QStringList *staticModuleList = nullptr);

    ImportedTypes builtinInternalNames();

    QList<QQmlJS::DiagnosticMessage> takeWarnings()
    {
        const auto result = std::move(m_warnings);
        m_warnings.clear();
        return result;
    }

    QList<QQmlJS::DiagnosticMessage> takeGlobalWarnings()
    {
        const auto result = std::move(m_globalWarnings);
        m_globalWarnings.clear();
        return result;
    }

    QStringList importPaths() const { return m_importPaths; }
    void setImportPaths(const QStringList &importPaths);

    void clearCache();

    QQmlJSScope::ConstPtr jsGlobalObject() const;

    std::unique_ptr<QQmlJSImportVisitor>
    makeImportVisitor(const QQmlJSScope::Ptr &target, QQmlJSImporter *importer,
                      QQmlJSLogger *logger, const QString &implicitImportDirectory,
                      const QStringList &qmldirFiles = QStringList());
    using ImportVisitorCreator = QQmlJSImportVisitor *(*)(const QQmlJSScope::Ptr &,
                                                          QQmlJSImporter *, QQmlJSLogger *,
                                                          const QString &, const QStringList &);
    void setImportVisitorCreator(ImportVisitorCreator create) { m_createImportVisitor = create; }

private:
    friend class QDeferredFactory<QQmlJSScope>;

    struct AvailableTypes
    {
        AvailableTypes(ImportedTypes builtins)
            : cppNames(std::move(builtins))
            , qmlNames(QQmlJS::ContextualTypes::QML, {}, cppNames.arrayType())
        {
        }

        // C++ names used in qmltypes files for non-composite types
        ImportedTypes cppNames;

        // Names the importing component sees, including any prefixes
        ImportedTypes qmlNames;

        // Static modules included here
        QStringList staticModules;

        // Whether a system module has been imported
        bool hasSystemModule = false;
    };

    struct Import {
        QString name;
        bool isStaticModule = false;
        bool isSystemModule = false;

        QList<QQmlJSExportedScope> objects;
        QHash<QString, QQmlJSExportedScope> scripts;
        QList<QQmlDirParser::Import> imports;
        QList<QQmlDirParser::Import> dependencies;
    };

    AvailableTypes builtinImportHelper();
    bool importHelper(const QString &module, AvailableTypes *types,
                      const QString &prefix = QString(), QTypeRevision version = QTypeRevision(),
                      bool isDependency = false, bool isFile = false);
    void processImport(const QQmlJS::Import &importDescription, const Import &import,
                       AvailableTypes *types);
    void importDependencies(const QQmlJSImporter::Import &import, AvailableTypes *types,
                            const QString &prefix = QString(),
                            QTypeRevision version = QTypeRevision(), bool isDependency = false);
    void readQmltypes(const QString &filename, QList<QQmlJSExportedScope> *objects,
                      QList<QQmlDirParser::Import> *dependencies);
    Import readQmldir(const QString &dirname);
    Import readDirectory(const QString &directory);

    QQmlJSScope::Ptr localFile2ScopeTree(const QString &filePath);
    static void setQualifiedNamesOn(const Import &import);

    QStringList m_importPaths;

    QHash<QPair<QString, QTypeRevision>, QString> m_seenImports;
    QHash<QQmlJS::Import, QSharedPointer<AvailableTypes>> m_cachedImportTypes;
    QHash<QString, Import> m_seenQmldirFiles;

    QHash<QString, QQmlJSScope::Ptr> m_importedFiles;
    QList<QQmlJS::DiagnosticMessage> m_globalWarnings;
    QList<QQmlJS::DiagnosticMessage> m_warnings;
    std::optional<AvailableTypes> m_builtins;

    QQmlJSResourceFileMapper *m_mapper = nullptr;
    QQmlJSResourceFileMapper *m_metaDataMapper = nullptr;
    bool m_useOptionalImports;

    ImportVisitorCreator m_createImportVisitor = nullptr;
};

QT_END_NAMESPACE

#endif // QQMLJSIMPORTER_P_H
