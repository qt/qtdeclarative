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

#ifndef QQUICK1ACCESSIBLEATTACHED_H
#define QQUICK1ACCESSIBLEATTACHED_H

#include <qdeclarativeitem.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

#ifndef QT_NO_ACCESSIBILITY

#include <private/qdeclarativeglobal_p.h>
#include <QtGui/qaccessible.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class Q_QTQUICK1_EXPORT QDeclarativeAccessibleAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAccessible::Role role READ role WRITE setRole NOTIFY roleChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)

public:
    Q_ENUMS(QAccessible::Role QAccessible::Event QAccessible::State)

    QDeclarativeAccessibleAttached(QObject *parent);
    ~QDeclarativeAccessibleAttached();

    QAccessible::Role role() const { return m_role; }
    void setRole(QAccessible::Role role)
    {
        m_role = role;
        emit roleChanged();
        // There is no way to signify role changes at the moment.
        // QAccessible::updateAccessibility(parent(), 0, QAccessible::);
    }
    QString name() const { return m_name; }
    void setName(const QString &name)
    {
        m_name = name;
        emit nameChanged();
        QAccessible::updateAccessibility(parent(), 0, QAccessible::NameChanged);
    }

    QString description() const { return m_description; }
    void setDescription(const QString &description)
    {
        m_description = description;
        emit descriptionChanged();
        QAccessible::updateAccessibility(parent(), 0, QAccessible::DescriptionChanged);
    }

    // Factory function
    static QDeclarativeAccessibleAttached *qmlAttachedProperties(QObject *);

    // Property getter
    static QObject *attachedProperties(const QObject *obj)
    {
        return qmlAttachedPropertiesObject<QDeclarativeAccessibleAttached>(obj, false);
    }

    static QVariant property(const QObject *object, const char *propertyName)
    {
        if (QObject *attachedObject = QDeclarativeAccessibleAttached::attachedProperties(object))
            return attachedObject->property(propertyName);
        return QVariant();
    }

    static bool setProperty(QObject *object, const char *propertyName, const QVariant &value)
    {
        QObject *obj = qmlAttachedPropertiesObject<QDeclarativeAccessibleAttached>(object, true);
        if (!obj) {
            qWarning("cannot set property Accessible.%s of QObject %s", propertyName, object->metaObject()->className());
            return false;
        }
        return obj->setProperty(propertyName, value);
    }


Q_SIGNALS:
    void roleChanged();
    void nameChanged();
    void descriptionChanged();
private:
    QAccessible::Role m_role;
    QString m_name;
    QString m_description;
public:
    using QObject::property;
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeAccessibleAttached)
QML_DECLARE_TYPEINFO(QDeclarativeAccessibleAttached, QML_HAS_ATTACHED_PROPERTIES)

QT_END_HEADER

#endif // QT_NO_ACCESSIBILITY

#endif

