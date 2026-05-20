// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#ifndef QQMLJSLINTERCONTEXT_P_H
#define QQMLJSLINTERCONTEXT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qset.h>
#include <QtCore/qstring.h>

#include <private/qduplicatetracker_p.h>
#include <private/qqmljscontextproperties_p.h>
#include <private/qqmljslinterrenamedcomponents_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljsscopesbyid_p.h>
#include <private/qqmljsusercontextproperties_p.h>
#include <private/qqmltoolingsettings_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

struct LinterContext
{
    Q_DISABLE_COPY_MOVE(LinterContext)
public:
    LinterContext(const QQmlJSScopesById &scopesById,
                  QDuplicateTracker<QQmlJSScope::ConstPtr> &knownUnresolvedTypes,
                  const QQmlJS::LinterRenamedComponents &renamedComponents,
                  const QQmlJSImporter &importer,
                  const QQmlJS::UserContextProperties &userContextProperties,
                  const QQmlJS::HeuristicContextProperties &heuristicContextProperties)
        : scopesById(scopesById),
          knownUnresolvedTypes(knownUnresolvedTypes),
          renamedComponents(renamedComponents),
          importer(importer),
          userContextProperties(userContextProperties),
          heuristicContextProperties(heuristicContextProperties)
    {
    }

    // begin group: copied data from LinterVisitor:
    QQmlJSScopesById scopesById = { };
    // end group: copied data from LinterVisitor:

    // begin group: data owned by LinterVisitor, does not outlive LinterVisitor.
    QDuplicateTracker<QQmlJSScope::ConstPtr> &knownUnresolvedTypes;
    const QQmlJS::LinterRenamedComponents &renamedComponents;
    // end group: data owned by LinterVisitor, does not outlive LinterVisitor.

    // begin group: data owned by QQmlJSLinter, does not outlive QQmlJSLinter
    const QQmlJSImporter &importer;
    const QQmlJS::UserContextProperties &userContextProperties;
    const QQmlJS::HeuristicContextProperties &heuristicContextProperties;
    // end group: data owned by QQmlJSLinter, does not outlive QQmlJSLinter
};

} // namespace QQmlJS

QT_END_NAMESPACE
#endif // QQMLJSLINTERCONTEXT_P_H
