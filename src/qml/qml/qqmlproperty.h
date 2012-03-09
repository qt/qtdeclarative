/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLPROPERTY_H
#define QQMLPROPERTY_H

#include <QtQml/qtqmlglobal.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QObject;
class QVariant;
class QQmlContext;
class QQmlEngine;

class QQmlPropertyPrivate;
class Q_QML_EXPORT QQmlProperty
{
public:
    enum PropertyTypeCategory {
        InvalidCategory,
        List,
        Object,
        Normal
    };

    enum Type { 
        Invalid,
        Property,
        SignalProperty
    };

    QQmlProperty();
    ~QQmlProperty();

    QQmlProperty(QObject *);
    QQmlProperty(QObject *, QQmlContext *);
    QQmlProperty(QObject *, QQmlEngine *);

    QQmlProperty(QObject *, const QString &);
    QQmlProperty(QObject *, const QString &, QQmlContext *);
    QQmlProperty(QObject *, const QString &, QQmlEngine *);

    QQmlProperty(const QQmlProperty &);
    QQmlProperty &operator=(const QQmlProperty &);

    bool operator==(const QQmlProperty &) const;

    Type type() const;
    bool isValid() const;
    bool isProperty() const;
    bool isSignalProperty() const;

    int propertyType() const;
    PropertyTypeCategory propertyTypeCategory() const;
    const char *propertyTypeName() const;

    QString name() const;

    QVariant read() const;
    static QVariant read(QObject *, const QString &);
    static QVariant read(QObject *, const QString &, QQmlContext *);
    static QVariant read(QObject *, const QString &, QQmlEngine *);

    bool write(const QVariant &) const;
    static bool write(QObject *, const QString &, const QVariant &);
    static bool write(QObject *, const QString &, const QVariant &, QQmlContext *);
    static bool write(QObject *, const QString &, const QVariant &, QQmlEngine *);

    bool reset() const;

    bool hasNotifySignal() const;
    bool needsNotifySignal() const;
    bool connectNotifySignal(QObject *dest, const char *slot) const;
    bool connectNotifySignal(QObject *dest, int method) const;

    bool isWritable() const;
    bool isDesignable() const;
    bool isResettable() const;
    QObject *object() const;

    int index() const;
    QMetaProperty property() const;
    QMetaMethod method() const;

private:
    friend class QQmlPropertyPrivate;
    QQmlPropertyPrivate *d;
};
typedef QList<QQmlProperty> QQmlProperties;

inline uint qHash (const QQmlProperty &key)
{
    return qHash(key.object()) + qHash(key.name());
}

Q_DECLARE_TYPEINFO(QQmlProperty, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQMLPROPERTY_H
