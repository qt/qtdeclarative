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

#include "qmltcvisitor.h"

#include <QtCore/qfileinfo.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

static QString uniqueNameFromPieces(const QStringList &pieces, QHash<QString, int> &repetitions)
{
    QString possibleName = pieces.join(u'_');
    const int count = repetitions[possibleName]++;
    if (count > 0)
        possibleName.append(u"_" + QString::number(count));
    return possibleName;
}

QmltcVisitor::QmltcVisitor(QQmlJSImporter *importer, QQmlJSLogger *logger,
                           const QString &implicitImportDirectory, const QStringList &qmltypesFiles)
    : QQmlJSImportVisitor(importer, logger, implicitImportDirectory, qmltypesFiles)
{
    m_qmlTypeNames.append(QFileInfo(logger->fileName()).baseName()); // put document root
}

bool QmltcVisitor::visit(QQmlJS::AST::UiObjectDefinition *object)
{
    if (!QQmlJSImportVisitor::visit(object))
        return false;

    Q_ASSERT(m_currentScope->scopeType() == QQmlJSScope::QMLScope);
    Q_ASSERT(m_currentScope->internalName().isEmpty());
    Q_ASSERT(!m_currentScope->baseTypeName().isEmpty());

    if (m_currentScope != m_exportedRootScope) // not document root
        m_qmlTypeNames.append(m_currentScope->baseTypeName());

    // give C++-relevant internal names to QMLScopes, we can use them later in compiler
    m_currentScope->setInternalName(uniqueNameFromPieces(m_qmlTypeNames, m_qmlTypeNameCounts));

    return true;
}

void QmltcVisitor::endVisit(QQmlJS::AST::UiObjectDefinition *)
{
    m_qmlTypeNames.removeLast();
}

QT_END_NAMESPACE
