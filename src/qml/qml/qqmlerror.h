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

#ifndef QQMLERROR_H
#define QQMLERROR_H

#include <QtQml/qtqmlglobal.h>

#include <QtCore/qurl.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

// ### Qt 6: should this be called QQmlMessage, since it can have a message type?
class QDebug;
class QQmlErrorPrivate;
class Q_QML_EXPORT QQmlError
{
public:
    QQmlError();
    QQmlError(const QQmlError &);
    QQmlError &operator=(const QQmlError &);
    ~QQmlError();

    bool isValid() const;

    QUrl url() const;
    void setUrl(const QUrl &);
    QString description() const;
    void setDescription(const QString &);
    int line() const;
    void setLine(int);
    int column() const;
    void setColumn(int);

    QObject *object() const;
    void setObject(QObject *);

    QtMsgType messageType() const;
    void setMessageType(QtMsgType messageType);

    QString toString() const;
private:
    QQmlErrorPrivate *d;
};

QDebug Q_QML_EXPORT operator<<(QDebug debug, const QQmlError &error);

Q_DECLARE_TYPEINFO(QQmlError, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif // QQMLERROR_H
