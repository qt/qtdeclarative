/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtScript module of the Qt Toolkit.
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
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSCRIPTVALUEITERATOR_H
#define QSCRIPTVALUEITERATOR_H

#include <QtDeclarative/qjsvalue.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Script)

class QString;

class QJSValueIteratorPrivate;
class Q_SCRIPT_EXPORT QJSValueIterator
{
public:
    QJSValueIterator(const QJSValue &value);
    ~QJSValueIterator();

    bool hasNext() const;
    bool next();

    QString name() const;

    QJSValue value() const;
    QJSValueIterator& operator=(QJSValue &value);

private:
    QScopedPointer<QJSValueIteratorPrivate> d_ptr;

    Q_DECLARE_PRIVATE(QJSValueIterator)
    Q_DISABLE_COPY(QJSValueIterator)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSCRIPTVALUEITERATOR_H
