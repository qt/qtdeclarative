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

#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "qmltctyperesolver.h"
#include "qmltcoutputir.h"
#include "prototype/qml2cppcontext.h"

#include <QtCore/qlist.h>
#include <QtCore/qqueue.h>
#include <QtCore/qhash.h>
#include <QtCore/qset.h>

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljscompiler_p.h>

#include <variant>
#include <utility>

QT_BEGIN_NAMESPACE

struct QmltcCompilerInfo;
class CodeGenerator
{
public:
    CodeGenerator(const QString &url, QQmlJSLogger *logger, const QmltcTypeResolver *localResolver,
                  const QmltcVisitor *visitor, const QmltcCompilerInfo *info);

    // initializes code generator
    void prepare(QSet<QString> *requiredCppIncludes,
                 const QSet<QQmlJSScope::ConstPtr> &suitableTypes);

private:
    QString m_url; // document url
    QQmlJSLogger *m_logger = nullptr;
    const QmltcTypeResolver *m_localTypeResolver = nullptr;
    const QmltcVisitor *m_visitor = nullptr;

    const QmltcCompilerInfo *m_info = nullptr;

private:
    // helper methods:
    void recordError(const QQmlJS::SourceLocation &location, const QString &message);
    void recordError(const QV4::CompiledData::Location &location, const QString &message);
};

QT_END_NAMESPACE

#endif // CODEGENERATOR_H
