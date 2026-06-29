// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QMLJSLINTER_P_H
#define QMLJSLINTER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmljslintervisitor_p.h"
#include <private/qqmljscontextproperties_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qqmljslintertypepropagator_p.h>
#include <private/qqmljslogger_p.h>
#include <private/qqmljssourcelocation_p.h>
#include <private/qqmljstypepropagator_p.h>
#include <private/qqmljsusercontextproperties_p.h>
#include <private/qqmltoolingsettings_p.h>

#include <QtCore/qjsonarray.h>
#include <QtCore/qstring.h>
#include <QtCore/qmap.h>
#include <QtCore/qscopedpointer.h>

#include <vector>

QT_BEGIN_NAMESPACE

class QPluginLoader;
struct QStaticPlugin;

namespace QQmlSA {
class LintPlugin;
}

class QQmlJSLinter
{
public:
    QQmlJSLinter(const QStringList &importPaths, const QStringList &extraPluginPaths = {},
                 bool useAbsolutePath = false);

    enum LintResult { FailedToOpen, FailedToParse, HasWarnings, HasErrors, LintSuccess };
    enum LintOption : quint8 {
        Silent = 1,
        GenerateJson = 2,
    };
    Q_DECLARE_FLAGS(LintOptions, LintOption);
    enum FixResult { NothingToFix, FixError, FixSuccess };

    class Plugin
    {
        Q_DISABLE_COPY(Plugin)
    public:
        Plugin() = default;
        Plugin(Plugin &&plugin) noexcept;

#if QT_CONFIG(library)
        Plugin(QString path);
#endif
        Plugin(const QStaticPlugin &plugin);
        ~Plugin();

        const QString &name() const { return m_name; }
        const QString &description() const { return m_description; }
        const QString &version() const { return m_version; }
        const QString &author() const { return m_author; }
        const QList<QQmlJS::LoggerCategory> categories() const
        {
            return m_categories;
        }
        bool isValid() const { return m_isValid; }
        bool isInternal() const
        {
            return m_isInternal;
        }

        bool isEnabled() const
        {
            return m_isEnabled;
        }
        void setEnabled(bool isEnabled)
        {
            m_isEnabled = isEnabled;
        }

    private:
        friend class QQmlJSLinter;

        bool parseMetaData(const QJsonObject &metaData, QString pluginName);

        QString m_name;
        QString m_description;
        QString m_version;
        QString m_author;

        QList<QQmlJS::LoggerCategory> m_categories;
        QQmlSA::LintPlugin *m_instance;
        std::unique_ptr<QPluginLoader> m_loader;
        bool m_isInternal =
                false; // Internal plugins are those developed and maintained inside the Qt project
        bool m_isValid = false;
        bool m_isEnabled = true;
    };

    static std::vector<Plugin> loadPlugins(QStringList paths);

    struct Result
    {
        LintResult status = LintSuccess;
        QJsonObject json;
        std::unique_ptr<QQmlJSLogger> logger;

        void generateJson();
        void setStatusFromLogger();
    };
    Result lintFileInBatch(const QString &filename);

    bool prepareFileForBatchLinting(const QString &filename, const QString *fileContents,
                                    LintOptions options, const QStringList &qmlImportPaths,
                                    const QStringList &qmldirFiles,
                                    const QStringList &resourceFiles,
                                    const QList<QQmlJS::LoggerCategory> &categories);

    Result lintModule(const QString &uri, LintOptions options, const QStringList &qmlImportPaths,
                      const QStringList &resourceFiles);

    static FixResult applyFixes(const QQmlJSLogger *logger, QString *fixedCode, bool silent);

    std::vector<Plugin> &plugins()
    {
        return m_plugins;
    }
    void setPlugins(std::vector<Plugin> plugins) { m_plugins = std::move(plugins); }

    void setPluginsEnabled(bool enablePlugins) { m_enablePlugins = enablePlugins; }
    bool pluginsEnabled() const { return m_enablePlugins; }
    bool useAbsolutePath() const { return m_useAbsolutePath; }

    void clearCache() { m_importer.clearCache(); }

private:
    void lintFileImpl(const QString &filename);
    Result lintModuleImpl(const QString &uri, LintOptions options,
                          const QStringList &qmlImportPaths, const QStringList &resourceFiles);
    void setupLoggingCategoriesInLogger(QQmlJSLogger *logger,
                                        const QList<QQmlJS::LoggerCategory> &categories);
    void parseComments(QQmlJSLogger *logger, const QList<QQmlJS::SourceLocation> &comments);
    void updateUserContextProperties(const QString &fileNamej);
    void updateHeuristicContextProperties(const QString &fileName);
    void typeReader(const QString &filename);

    bool m_useAbsolutePath;
    bool m_enablePlugins;
protected:
    QQmlJSImporter m_importer;
private:
    QString m_fileContents;
    std::vector<Plugin> m_plugins;

    struct LintInfo
    {
        const QString *fileContents;
        LintOptions options;
        QStringList qmlImportPaths;
        QStringList qmldirFiles;
        std::optional<QQmlJSResourceFileMapper> resourceMapper;
        QList<QQmlJS::LoggerCategory> categories;

        QQmlJSScope::Ptr handle;
        std::optional<QQmlJS::LinterVisitor> visitor;
        Result result;

        QQmlJS::Engine engine; // needs to outlive the lintFileInBatch() call.
    };

    std::unordered_map<QString, LintInfo> m_lintInfo;

    QQmlToolingSettings m_userContextPropertySettings =
            QQmlToolingSettings(QStringLiteral("contextProperties"));
    QQmlJS::UserContextProperties m_cachedUserContextProperties;
    QQmlToolingSettings::Searcher m_heuristicContextPropertySearcher =
            QQmlToolingSettings::Searcher(QStringLiteral(".qt/contextPropertyDump.ini"),
                                          QStringLiteral("contextPropertyDump.ini"));
    QQmlJS::HeuristicContextProperties m_cachedHeuristicContextProperties;
};

QT_END_NAMESPACE

#endif // QMLJSLINTER_P_H
