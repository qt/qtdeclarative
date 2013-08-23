/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <private/qv4codegen_p.h>
#include <private/qqmlpool_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QVariant>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QLibraryInfo>
#include <QtCore/QDebug>

#include <iostream>

QT_USE_NAMESPACE

QStringList g_qmlImportPaths;

void printUsage(const QString &appName)
{
    qWarning() << qPrintable(QString(
                                 "Usage: %1 -rootPath qmldir -importPath importPath \n"
                                 "Example: %1 -rootPath qmldir -importPath importPath").arg(
                                 appName));
}

QVariantList findImportsInAst(QQmlJS::AST::UiHeaderItemList *headerItemList, const QByteArray &code, const QString &path) {
    QVariantList imports;

    // extract uri and version from the imports (which look like "import Foo.Bar 1.2.3")
    for (QQmlJS::AST::UiHeaderItemList *headerItemIt = headerItemList; headerItemIt; headerItemIt = headerItemIt->next) {
        QVariantMap import;
        QQmlJS::AST::UiImport *importNode = QQmlJS::AST::cast<QQmlJS::AST::UiImport *>(headerItemIt->headerItem);
        if (!importNode)
            continue;
        // handle directory imports
        if (!importNode->fileName.isEmpty()) {
            QString name = importNode->fileName.toString();
            import["name"] = name;
            import["type"] = QStringLiteral("directory");
            import["path"] = path + QStringLiteral("/") + name;
        } else {
            // Walk the id chain ("Foo" -> "Bar" -> etc)
            QString  name;
            QQmlJS::AST::UiQualifiedId *uri = importNode->importUri;
            while (uri) {
                name.append(uri->name);
                name.append(".");
                uri = uri->next;
            }
            name.chop(1); // remove trailing "."
            if (!name.isEmpty())
                import["name"] = name;
            import["type"] = QStringLiteral("module");
            import["version"] = code.mid(importNode->versionToken.offset, importNode->versionToken.length);
        }

        imports.append(import);
    }

    return imports;
}

// Scan a single qml file for import statements
QVariantList findQmlImportsInFile(const QString &qmlFilePath) {
    QFile qmlFile(qmlFilePath);
    if (!qmlFile.open(QIODevice::ReadOnly)) {
           std::cerr << "Cannot open input file " << qPrintable(qmlFile.fileName()) << std::endl;
           return QVariantList();
    }
    QByteArray code = qmlFile.readAll();

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(code, /*line = */ 1);
    QQmlJS::Parser parser(&engine);

    if (!parser.parse() || !parser.diagnosticMessages().isEmpty()) {
        // Extract errors from the parser
        foreach (const QQmlJS::DiagnosticMessage &m, parser.diagnosticMessages()) {
            if (m.isWarning()) {
                qWarning("%s:%d : %s", qPrintable(qmlFile.fileName()), m.loc.startLine, qPrintable(m.message));
                continue;
            }
            std::cerr << qPrintable(qmlFile.fileName()) << ":" << m.loc.startLine << ":" << qPrintable(m.message) << std::endl;
        }
        return QVariantList();
    }

    return findImportsInAst(parser.ast()->headers, code, QFileInfo(qmlFilePath).absolutePath());
}

// Scan all qml files in directory for import statements
QVariantList findQmlImportsInDirectory(const QString &qmlDir)
{
    QVariantList ret;
    if (qmlDir.isEmpty())
        return ret;

    QStringList qmlFileNames = QDir(qmlDir).entryList(QStringList() << "*.qml");
    foreach (const QString &qmlFileName, qmlFileNames) {
        QString qmlFilePath = qmlDir + "/" + qmlFileName;
            QVariantList imports = findQmlImportsInFile(qmlFilePath);
            ret.append(imports);

    }
    return ret;
}

// Read the qmldir file, extract a list of plugins by
// parsing the "plugin" lines
QString pluginsForModulePath(const QString &modulePath) {
    QFile qmldirFile(modulePath + "/qmldir");
    if (!qmldirFile.exists()) {
        return QString();
    }
    qmldirFile.open(QIODevice::ReadOnly | QIODevice::Text);

    QString plugins;
    QByteArray line;
    do {
        line = qmldirFile.readLine();
        if (line.startsWith("plugin")) {
            plugins += QString::fromUtf8(line.split(' ').at(1));
            plugins += " ";
        }
    } while (line.length() > 0);

    return plugins.simplified();
}

// Construct a file system path from a module uri and version.
// Special case for 1.x: QtQuick.Dialogs 1.x -> QtQuick/Dialogs
// Genral casefor y.x:  QtQuick.Dialogs y.x -> QtQuick/Dialogs.y
QString localPathForModule(const QString &moduleUri, const QString &version) {
    QString path;
    foreach (const QString &part, moduleUri.split(".")) {
        path += QString("/" + part);
    }

    if (version.startsWith("1."))
        return path;

    if (version.contains("."))
        path += "." + version.split(".").at(0);
    else
        path += "." + version;
    return path;
}

// Search for a given qml import in g_qmlImportPaths
QString findPathForImport(const QString&localModulePath) {
    foreach (const QString &qmlImportPath, g_qmlImportPaths) {
        QString candidatePath = QDir::cleanPath(qmlImportPath + "/" + localModulePath);
        if (QDir(candidatePath).exists())
            return candidatePath;
    }
    return QString();
}

// Find absolute file system paths and plugins for a list of modules.
QVariantList findPathsForModuleImports(const QVariantList &imports) {
    QVariantList done;

    foreach (QVariant importVariant, imports) {
        QVariantMap import = qvariant_cast<QVariantMap>(importVariant);
        if (import["type"] == QStringLiteral("module")) {
            import["path"] = findPathForImport(localPathForModule(import["name"].toString(), import["version"].toString()));
            QString plugin = pluginsForModulePath(import["path"].toString());
            if (!plugin.isEmpty())
                import["plugin"] = plugin;
        }
        if (!import["path"].isNull())
            done.append(import);
    }
    return done;
}

// Merge two lists of imports, discard duplicates.
QVariantList mergeImports(const QVariantList &a, const QVariantList &b) {
    QVariantList merged = a;
    foreach (const QVariant &variant, b) {
        if (!merged.contains(variant))
            merged.append(variant);
    }
    return merged;
}

// find Qml Imports Recursively
QVariantList findQmlImportsRecursively(const QStringList &qmlDirs) {
    QVariantList ret;

    QSet<QString> toVisit = qmlDirs.toSet();
    QSet<QString> visited;
    while (!toVisit.isEmpty()) {
        QString qmlDir = *toVisit.begin();
        toVisit.erase(toVisit.begin());
        visited.insert(qmlDir);

        QVariantList imports = findQmlImportsInDirectory(qmlDir);
        imports = findPathsForModuleImports(imports);

        // schedule recursive visit of imports
        foreach (const QVariant &importVariant, imports) {
            QVariantMap import = qvariant_cast<QVariantMap>(importVariant);
            QString path = import["path"].toString();
            if (!path.isEmpty() && !visited.contains(path)) {
                toVisit.insert(path);
            }
        }

        ret = mergeImports(ret, imports);
    }

    return ret;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    const QString appName = QFileInfo(app.applicationFilePath()).baseName();
    if (args.size() < 1) {
        printUsage(appName);
        return 1;
    }

    QStringList qmlRootPaths;
    QStringList qmlImportPaths;

    int i = 1;
    while (i < args.count()) {
        const QString &arg = args.at(i);
        ++i;
        if (!arg.startsWith("-")) {
            qmlRootPaths += arg;
        } else if (arg == QLatin1String("-rootPath")) {
            if (i >= args.count())
                qWarning() << "-rootPath requires an argument";

            while (i < args.count()) {
                const QString arg = args.at(i);
                if (arg.startsWith("-"))
                    break;
                ++i;
                qmlRootPaths += arg;
            }
        } else if (arg == QLatin1String("-importPath")) {
            if (i >= args.count())
                qWarning() << "-importPath requires an argument";

            while (i < args.count()) {
                const QString arg = args.at(i);
                if (arg.startsWith("-"))
                    break;
                ++i;
                qmlImportPaths += arg;
            }
        } else {
            qWarning() << "Invalid argument: " << arg;
            return 1;
        }
    }

    g_qmlImportPaths = qmlImportPaths;

    // Find the imports!
    QVariantList imports = findQmlImportsRecursively(qmlRootPaths);

    // Convert to JSON
    QByteArray json = QJsonDocument(QJsonArray::fromVariantList(imports)).toJson();
    std::cout << json.constData() << std::endl;
    return 0;
}
