/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEOPENMETAOBJECT_H
#define QDECLARATIVEOPENMETAOBJECT_H

#include <QtCore/QMetaObject>
#include <QtCore/QObject>

#include <QtDeclarative/private/qdeclarativerefcount_p.h>
#include <QtDeclarative/private/qdeclarativeglobal_p.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarativeEngine;
class QMetaPropertyBuilder;

QT_MODULE(Declarative)

class QDeclarative1OpenMetaObjectTypePrivate;
class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarative1OpenMetaObjectType : public QDeclarativeRefCount
{
public:
    QDeclarative1OpenMetaObjectType(const QMetaObject *base, QDeclarativeEngine *engine);
    ~QDeclarative1OpenMetaObjectType();

    int createProperty(const QByteArray &name);

    int propertyOffset() const;
    int signalOffset() const;

protected:
    virtual void propertyCreated(int, QMetaPropertyBuilder &);

private:
    QDeclarative1OpenMetaObjectTypePrivate *d;
    friend class QDeclarative1OpenMetaObject;
    friend class QDeclarative1OpenMetaObjectPrivate;
};

class QDeclarative1OpenMetaObjectPrivate;
class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarative1OpenMetaObject : public QAbstractDynamicMetaObject
{
public:
    QDeclarative1OpenMetaObject(QObject *, bool = true);
    QDeclarative1OpenMetaObject(QObject *, QDeclarative1OpenMetaObjectType *, bool = true);
    ~QDeclarative1OpenMetaObject();

    QVariant value(const QByteArray &) const;
    void setValue(const QByteArray &, const QVariant &);
    QVariant value(int) const;
    void setValue(int, const QVariant &);
    QVariant &operator[](const QByteArray &);
    QVariant &operator[](int);
    bool hasValue(int) const;

    int count() const;
    QByteArray name(int) const;

    QObject *object() const;
    virtual QVariant initialValue(int);

    // Be careful - once setCached(true) is called createProperty() is no
    // longer automatically called for new properties.
    void setCached(bool);

    QDeclarative1OpenMetaObjectType *type() const;

protected:
    virtual int metaCall(QMetaObject::Call _c, int _id, void **_a);
    virtual int createProperty(const char *, const char *);

    virtual void propertyRead(int);
    virtual void propertyWrite(int);
    virtual void propertyWritten(int);
    virtual void propertyCreated(int, QMetaPropertyBuilder &);

    QAbstractDynamicMetaObject *parent() const;

private:
    QDeclarative1OpenMetaObjectPrivate *d;
    friend class QDeclarative1OpenMetaObjectType;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVEOPENMETAOBJECT_H
