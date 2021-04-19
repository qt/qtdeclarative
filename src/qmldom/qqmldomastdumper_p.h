/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**/
#ifndef QQMLDOMASTDUMPER_P_H
#define QQMLDOMASTDUMPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqmldom_global.h"
#include "qqmldomconstants_p.h"
#include "qqmldomstringdumper_p.h"

#include <QtQml/private/qqmljsglobal_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QDebug;

namespace QQmlJS {
namespace Dom {

inline QStringView noStr(SourceLocation)
{
    return QStringView();
}

QMLDOM_EXPORT QString lineDiff(QString s1, QString s2, int nContext);
QMLDOM_EXPORT QString astNodeDiff(AST::Node *n1, AST::Node *n2, int nContext = 3,
                                  AstDumperOptions opt = AstDumperOption::None, int indent = 0,
                                  function_ref<QStringView(SourceLocation)> loc2str1 = noStr,
                                  function_ref<QStringView(SourceLocation)> loc2str2 = noStr);
QMLDOM_EXPORT void astNodeDumper(Sink s, AST::Node *n, AstDumperOptions opt = AstDumperOption::None,
                                 int indent = 1, int baseIndent = 0,
                                 function_ref<QStringView(SourceLocation)> loc2str = noStr);
QMLDOM_EXPORT QString astNodeDump(AST::Node *n, AstDumperOptions opt = AstDumperOption::None,
                                  int indent = 1, int baseIndent = 0,
                                  function_ref<QStringView(SourceLocation)> loc2str = noStr);

QMLDOM_EXPORT QDebug operator<<(QDebug d, AST::Node *n);

} // namespace Dom
} // namespace AST

QT_END_NAMESPACE

#endif // QQMLDOMASTDUMPER_P_H
