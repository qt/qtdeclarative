/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qqmljsscope_p.h"
#include <QtQml/private/qqmldirparser_p.h>

QT_BEGIN_NAMESPACE

class QQmlJSImporter
{
public:
    using ImportedTypes = QHash<QString, QQmlJSScope::ConstPtr>;

    QQmlJSImporter(const QStringList &importPaths) : m_importPaths(importPaths), m_builtins({}) {}

    ImportedTypes importBuiltins();
    ImportedTypes importQmltypes(const QStringList &qmltypesFiles);

    QQmlJSScope::Ptr importFile(const QString &file);
    ImportedTypes importDirectory(const QString &directory, const QString &prefix = QString());

    ImportedTypes importModule(
            const QString &module, const QString &prefix = QString(),
            QTypeRevision version = QTypeRevision());

    ImportedTypes builtinInternalNames();

    QList<QQmlJS::DiagnosticMessage> takeWarnings()
    {
        const auto result = std::move(m_warnings);
        m_warnings.clear();
        return result;
    }

private:
    friend class QDeferredFactory<QQmlJSScope>;

    struct AvailableTypes
    {
        AvailableTypes(QHash<QString, QQmlJSScope::ConstPtr> builtins)
            : cppNames(std::move(builtins))
        {}

        // C++ names used in qmltypes files for non-composite types
        QHash<QString, QQmlJSScope::ConstPtr> cppNames;

        // Names the importing component sees, including any prefixes
        QHash<QString, QQmlJSScope::ConstPtr> qmlNames;
    };

    struct Import {
        QHash<QString, QQmlJSScope::Ptr> objects;
        QHash<QString, QQmlJSScope::Ptr> scripts;
        QList<QQmlDirParser::Import> imports;
        QList<QQmlDirParser::Import> dependencies;
    };

    AvailableTypes builtinImportHelper();
    void importHelper(const QString &module, AvailableTypes *types,
                      const QString &prefix = QString(),
                      QTypeRevision version = QTypeRevision());
    void processImport(const Import &import, AvailableTypes *types,
                       const QString &prefix = QString());
    void importDependencies(const QQmlJSImporter::Import &import,
                            AvailableTypes *types,
                            const QString &prefix = QString(),
                            QTypeRevision version = QTypeRevision());
    void readQmltypes(const QString &filename, QHash<QString, QQmlJSScope::Ptr> *objects,
                      QList<QQmlDirParser::Import> *dependencies);
    Import readQmldir(const QString &dirname);
    QQmlJSScope::Ptr localFile2ScopeTree(const QString &filePath);

    QStringList m_importPaths;
    QHash<QPair<QString, QTypeRevision>, Import> m_seenImports;
    QHash<QString, QQmlJSScope::Ptr> m_importedFiles;
    QList<QQmlJS::DiagnosticMessage> m_warnings;
    AvailableTypes m_builtins;
};

QT_END_NAMESPACE

#endif // QQMLJSIMPORTER_P_H
