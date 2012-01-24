/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-ONLY$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** us via http://www.qt-project.org/.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QJSVALUEITERATOR_P_H
#define QJSVALUEITERATOR_P_H

#include <private/qintrusivelist_p.h>
#include "qjsvalue_p.h"

#include <private/qv8_p.h>

QT_BEGIN_NAMESPACE

class QV8Engine;

class QJSValueIteratorPrivate
{
public:
    inline QJSValueIteratorPrivate(const QJSValuePrivate* value);
    inline ~QJSValueIteratorPrivate();

    inline bool hasNext() const;
    inline bool next();

    inline QString name() const;

    inline QScriptPassPointer<QJSValuePrivate> value() const;

    inline bool isValid() const;
    inline QV8Engine* engine() const;

    inline void invalidate();
private:
    Q_DISABLE_COPY(QJSValueIteratorPrivate)

    QIntrusiveListNode m_node;
    QScriptSharedDataPointer<QJSValuePrivate> m_object;
    v8::Persistent<v8::Array> m_names;
    uint32_t m_index;
    uint32_t m_count;

    friend class QV8Engine;
};


QT_END_NAMESPACE

#endif // QJSVALUEITERATOR_P_H
