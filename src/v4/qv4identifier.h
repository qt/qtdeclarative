/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
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
#ifndef QV4IDENTIFIER_H
#define QV4IDENTIFIER_H

#include <qv4string.h>
#include <qmljs_engine.h>
#include <limits.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

struct Identifiers
{
    ExecutionEngine *engine;
    uint currentIndex;
    QHash<QString, String *> identifiers;
public:

    Identifiers(ExecutionEngine *engine) : engine(engine), currentIndex(0) {}

    String *insert(const QString &s)
    {
        QHash<QString, String*>::const_iterator it = identifiers.find(s);
        if (it != identifiers.constEnd())
            return it.value();

        String *str = engine->newString(s);
        str->createHashValue();
        if (str->subtype == String::StringType_ArrayIndex)
            return str;

        str->identifier = currentIndex;
        if (currentIndex <= USHRT_MAX) {
            identifiers.insert(s, str);
            ++currentIndex;
        }
        return str;
    }

    void toIdentifier(String *s) {
        if (s->identifier != UINT_MAX)
            return;
        if (s->subtype == String::StringType_Unknown)
            s->createHashValue();
        if (s->subtype == String::StringType_ArrayIndex)
            return;
        QHash<QString, String*>::const_iterator it = identifiers.find(s->toQString());
        if (it != identifiers.constEnd()) {
            s->identifier = (*it)->identifier;
            return;
        }
        s->identifier = currentIndex;
        if (currentIndex <= USHRT_MAX) {
            identifiers.insert(s->toQString(), s);
            ++currentIndex;
        }
    }

    void mark() {
        for (QHash<QString, String *>::const_iterator it = identifiers.constBegin(); it != identifiers.constEnd(); ++it)
            (*it)->mark();
    }
};

} // namespace VM
} // namespace QQmlJS

QT_END_NAMESPACE

#endif
