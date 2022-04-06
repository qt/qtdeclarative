/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include <private/qtqmlcompilerexports_p.h>

#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQmlCompiler/private/qqmljsimporter_p.h>

#include <QtQml/private/qqmljssourcelocation_p.h>

#include <QtCore/qjsonarray.h>
#include <QtCore/qstring.h>
#include <QtCore/qmap.h>
#include <QtCore/qscopedpointer.h>

#include <vector>
#include <optional>

QT_BEGIN_NAMESPACE

class QPluginLoader;
struct QStaticPlugin;

namespace QQmlSA {
class LintPlugin;
}

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSLinter
{
public:
    QQmlJSLinter(const QStringList &importPaths,
                 const QStringList &pluginPaths = { QQmlJSLinter::defaultPluginPath() },
                 bool useAbsolutePath = false);

    enum LintResult { FailedToOpen, FailedToParse, HasWarnings, LintSuccess };
    enum FixResult { NothingToFix, FixError, FixSuccess };

    class Q_QMLCOMPILER_PRIVATE_EXPORT Plugin
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
        bool isBuiltin() const { return m_isBuiltin; }
        bool isValid() const { return m_isValid; }

    private:
        friend class QQmlJSLinter;

        bool parseMetaData(const QJsonObject &metaData, QString pluginName);

        QString m_name;
        QString m_description;
        QString m_version;
        QString m_author;

        QQmlSA::LintPlugin *m_instance;
        QPluginLoader *m_loader = nullptr;
        bool m_isBuiltin;
        bool m_isValid = false;
    };

    static std::vector<Plugin> loadPlugins(QStringList paths);
    static QString defaultPluginPath();

    LintResult lintFile(const QString &filename, const QString *fileContents, const bool silent,
                        QJsonArray *json, const QStringList &qmlImportPaths,
                        const QStringList &qmldirFiles, const QStringList &resourceFiles,
                        const QMap<QString, QQmlJSLogger::Option> &options);

    FixResult applyFixes(QString *fixedCode, bool silent);

    const QQmlJSLogger *logger() const { return m_logger.get(); }

    const std::vector<Plugin> &plugins() const { return m_plugins; }
    void setPlugins(std::vector<Plugin> plugins) { m_plugins = std::move(plugins); }

    void setPluginsEnabled(bool enablePlugins) { m_enablePlugins = enablePlugins; }
    bool pluginsEnabled() const { return m_enablePlugins; }

private:
    void parseComments(QQmlJSLogger *logger, const QList<QQmlJS::SourceLocation> &comments);

    bool m_useAbsolutePath;
    bool m_enablePlugins;
    QQmlJSImporter m_importer;
    QScopedPointer<QQmlJSLogger> m_logger;
    QString m_fileContents;
    std::vector<Plugin> m_plugins;
};

QT_END_NAMESPACE

#endif // QMLJSLINTER_P_H
