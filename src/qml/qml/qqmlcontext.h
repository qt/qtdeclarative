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

#ifndef QQMLCONTEXT_H
#define QQMLCONTEXT_H

#include <QtCore/qurl.h>
#include <QtCore/qobject.h>
#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtQml/qjsvalue.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE


class QString;
class QQmlEngine;
class QQmlRefCount;
class QQmlContextPrivate;
class QQmlCompositeTypeData;
class QQmlContextData;

class Q_QML_EXPORT QQmlContext : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlContext)

public:
    struct PropertyPair { QString name; QVariant value; };

    QQmlContext(QQmlEngine *parent, QObject *objParent = nullptr);
    QQmlContext(QQmlContext *parent, QObject *objParent = nullptr);
    ~QQmlContext() override;

    bool isValid() const;

    QQmlEngine *engine() const;
    QQmlContext *parentContext() const;

    QObject *contextObject() const;
    void setContextObject(QObject *);

    QVariant contextProperty(const QString &) const;
    void setContextProperty(const QString &, QObject *);
    void setContextProperty(const QString &, const QVariant &);
    void setContextProperties(const QVector<PropertyPair> &properties);

    // ### Qt 6: no need for a mutable object, this should become a const QObject pointer
    QString nameForObject(QObject *) const;

    QUrl resolvedUrl(const QUrl &);

    void setBaseUrl(const QUrl &);
    QUrl baseUrl() const;

private:
    friend class QQmlEngine;
    friend class QQmlEnginePrivate;
    friend class QQmlExpression;
    friend class QQmlExpressionPrivate;
    friend class QQmlComponent;
    friend class QQmlComponentPrivate;
    friend class QQmlScriptPrivate;
    friend class QQmlContextData;
    QQmlContext(QQmlContextData *);
    QQmlContext(QQmlEngine *, bool);
    Q_DISABLE_COPY(QQmlContext)
};
QT_END_NAMESPACE

Q_DECLARE_METATYPE(QList<QObject*>)

#endif // QQMLCONTEXT_H
