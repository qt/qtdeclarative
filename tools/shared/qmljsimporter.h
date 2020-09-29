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

#ifndef QMLJSIMPORTER_H
#define QMLJSIMPORTER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "scopetree.h"
#include <QtQml/private/qqmldirparser_p.h>

class QmlJSImporter
{
public:
    struct ImportedTypes
    {
        // C++ names used in qmltypes files for non-composite types
        QHash<QString, ScopeTree::Ptr> cppNames;

        // Names the importing component sees, including any prefixes
        QHash<QString, ScopeTree::Ptr> qmlNames;
    };

    QmlJSImporter(const QStringList &importPaths) : m_importPaths(importPaths) {}

    ImportedTypes importBuiltins();
    ImportedTypes importQmltypes(const QStringList &qmltypesFiles);
    ImportedTypes importFileOrDirectory(
            const QString &fileOrDirectory, const QString &prefix = QString());
    ImportedTypes importModule(
            const QString &module, const QString &prefix = QString(),
            QTypeRevision version = QTypeRevision());

    QStringList takeWarnings()
    {
        QStringList result = std::move(m_warnings);
        m_warnings.clear();
        return result;
    }

private:
    struct Import {
        QHash<QString, ScopeTree::Ptr> objects;
        QHash<QString, ScopeTree::Ptr> scripts;
        QList<QQmlDirParser::Import> imports;
        QList<QQmlDirParser::Import> dependencies;
    };

    void importHelper(const QString &module, ImportedTypes *types,
                      const QString &prefix = QString(),
                      QTypeRevision version = QTypeRevision());
    void processImport(const Import &import, ImportedTypes *types,
                       const QString &prefix = QString());
    void importDependencies(const QmlJSImporter::Import &import,
                            ImportedTypes *types,
                            const QString &prefix = QString(),
                            QTypeRevision version = QTypeRevision());
    void readQmltypes(const QString &filename, QHash<QString, ScopeTree::Ptr> *objects);
    Import readQmldir(const QString &dirname);
    ScopeTree::Ptr localFile2ScopeTree(const QString &filePath);

    QStringList m_importPaths;
    QHash<QPair<QString, QTypeRevision>, Import> m_seenImports;
    QHash<QString, ScopeTree::Ptr> m_importedFiles;
    QStringList m_warnings;
};

#endif // QMLJSIMPORTER_H
