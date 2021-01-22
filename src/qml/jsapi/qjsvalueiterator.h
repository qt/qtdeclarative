/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QJSVALUEITERATOR_H
#define QJSVALUEITERATOR_H

#include <QtQml/qjsvalue.h>
#include <QtQml/qtqmlglobal.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE


class QString;

class QJSValueIteratorPrivate;
class Q_QML_EXPORT QJSValueIterator
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

#endif // QJSVALUEITERATOR_H
