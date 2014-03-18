/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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
#ifndef QQMLSCRIPT_P_H
#define QQMLSCRIPT_P_H

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

#include <QtQml/qqmlerror.h>

#include <private/qfieldlist_p.h>
#include <private/qhashfield_p.h>
#include <private/qqmlpool_p.h>
#include <private/qqmlpropertycache_p.h>

#include <QtCore/QList>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE


class QByteArray;
class QQmlPropertyCache;
namespace QQmlJS { class Engine; namespace AST { class Node; class StringLiteral; class UiProgram; class FunctionDeclaration; } }

namespace QQmlScript {

class Object : public QQmlPool::Class
{
public:
    // Script blocks that were nested under this object
    struct ScriptBlock {
        enum Pragma {
            None   = 0x00000000,
            Shared = 0x00000001
        };
        Q_DECLARE_FLAGS(Pragmas, Pragma)

        QString code;
        QString file;
        Pragmas pragmas;
    };
};

class Q_QML_PRIVATE_EXPORT Parser
{
public:
    static QQmlScript::Object::ScriptBlock::Pragmas extractPragmas(QString &);
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(QQmlScript::Object::ScriptBlock::Pragmas)

QT_END_NAMESPACE

#endif // QQMLSCRIPT_P_H
