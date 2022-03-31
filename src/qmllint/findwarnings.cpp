/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "findwarnings_p.h"

#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlCompiler/private/qqmljstypedescriptionreader_p.h>
#include <QtQmlCompiler/private/qqmljstypereader_p.h>

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmlimportresolver_p.h>

#include <QtCore/qfile.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qscopedvaluerollback.h>

QT_BEGIN_NAMESPACE

FindWarningVisitor::FindWarningVisitor(
        const QQmlJSScope::Ptr &target, QQmlJSImporter *importer, QQmlJSLogger *logger,
        QStringList qmldirFiles, QList<QQmlJS::SourceLocation> comments)
    : QQmlJSImportVisitor(
            target, importer, logger,
            implicitImportDirectory(logger->fileName(), importer->resourceFileMapper()),
            qmldirFiles)
{
    parseComments(comments);
}

void FindWarningVisitor::parseComments(const QList<QQmlJS::SourceLocation> &comments)
{
    QHash<int, QSet<QQmlJSLoggerCategory>> disablesPerLine;
    QHash<int, QSet<QQmlJSLoggerCategory>> enablesPerLine;
    QHash<int, QSet<QQmlJSLoggerCategory>> oneLineDisablesPerLine;

    const QString code = m_logger->code();
    const QStringList lines = code.split(u'\n');

    for (const auto &loc : comments) {
        const QString comment = code.mid(loc.offset, loc.length);
        if (!comment.startsWith(u" qmllint ") && !comment.startsWith(u"qmllint "))
            continue;

        QStringList words = comment.split(u' ');
        if (words.constFirst().isEmpty())
            words.removeFirst();

        const QString command = words.at(1);

        QSet<QQmlJSLoggerCategory> categories;
        for (qsizetype i = 2; i < words.size(); i++) {
            const QString category = words.at(i);
            const auto option = m_logger->options().constFind(category);
            if (option != m_logger->options().constEnd())
                categories << option->m_category;
            else
                m_logger->logWarning(
                        u"qmllint directive on unknown category \"%1\""_qs.arg(category),
                        Log_Syntax, loc);
        }

        if (categories.isEmpty()) {
            for (const auto &option : m_logger->options())
                categories << option.m_category;
        }

        if (command == u"disable"_qs) {
            const QString line = lines[loc.startLine - 1];
            const QString preComment = line.left(line.indexOf(comment) - 2);

            bool lineHasContent = false;
            for (qsizetype i = 0; i < preComment.size(); i++) {
                if (!preComment[i].isSpace()) {
                    lineHasContent = true;
                    break;
                }
            }

            if (lineHasContent)
                oneLineDisablesPerLine[loc.startLine] |= categories;
            else
                disablesPerLine[loc.startLine] |= categories;
        } else if (command == u"enable"_qs) {
            enablesPerLine[loc.startLine + 1] |= categories;
        } else {
            m_logger->logWarning(u"Invalid qmllint directive \"%1\" provided"_qs.arg(command),
                                 Log_Syntax, loc);
        }
    }

    if (disablesPerLine.isEmpty() && oneLineDisablesPerLine.isEmpty())
        return;

    QSet<QQmlJSLoggerCategory> currentlyDisabled;
    for (qsizetype i = 1; i <= lines.length(); i++) {
        currentlyDisabled.unite(disablesPerLine[i]).subtract(enablesPerLine[i]);

        currentlyDisabled.unite(oneLineDisablesPerLine[i]);

        if (!currentlyDisabled.isEmpty())
            m_logger->ignoreWarnings(i, currentlyDisabled);

        currentlyDisabled.subtract(oneLineDisablesPerLine[i]);
    }
}

bool FindWarningVisitor::check()
{
    return !m_logger->hasWarnings() && !m_logger->hasErrors();
}

QT_END_NAMESPACE
