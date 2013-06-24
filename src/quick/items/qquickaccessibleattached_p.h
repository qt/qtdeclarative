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

#ifndef QQUICKACCESSIBLEATTACHED_H
#define QQUICKACCESSIBLEATTACHED_H

#include <QtQuick/qquickitem.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

#ifndef QT_NO_ACCESSIBILITY

#include <QtGui/qaccessible.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE


class Q_QUICK_PRIVATE_EXPORT QQuickAccessibleAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAccessible::Role role READ role WRITE setRole NOTIFY roleChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)

public:
    Q_ENUMS(QAccessible::Role QAccessible::Event QAccessible::State)

    QQuickAccessibleAttached(QObject *parent);
    ~QQuickAccessibleAttached();

    QAccessible::Role role() const { return m_role; }
    void setRole(QAccessible::Role role)
    {
        if (role != m_role) {
            m_role = role;
            emit roleChanged();
            // There is no way to signify role changes at the moment.
            // QAccessible::updateAccessibility(parent(), 0, QAccessible::);
        }
    }
    QString name() const { return m_name; }
    void setName(const QString &name) {
        if (name != m_name) {
            m_name = name;
            emit nameChanged();
            QAccessibleEvent ev(parent(), QAccessible::NameChanged);
            QAccessible::updateAccessibility(&ev);
        }
    }

    QString description() const { return m_description; }
    void setDescription(const QString &description)
    {
        if (m_description != description) {
            m_description = description;
            emit descriptionChanged();
            QAccessibleEvent ev(parent(), QAccessible::DescriptionChanged);
            QAccessible::updateAccessibility(&ev);
        }
    }

    // Factory function
    static QQuickAccessibleAttached *qmlAttachedProperties(QObject *obj);

    // Property getter
    static QObject *attachedProperties(const QObject *obj)
    {
        return qmlAttachedPropertiesObject<QQuickAccessibleAttached>(obj, false);
    }

    static QVariant property(const QObject *object, const char *propertyName)
    {
        if (QObject *attachedObject = QQuickAccessibleAttached::attachedProperties(object))
            return attachedObject->property(propertyName);
        return QVariant();
    }

    static bool setProperty(QObject *object, const char *propertyName, const QVariant &value)
    {
        QObject *obj = qmlAttachedPropertiesObject<QQuickAccessibleAttached>(object, true);
        if (!obj) {
            qWarning("cannot set property Accessible.%s of QObject %s", propertyName, object->metaObject()->className());
            return false;
        }
        return obj->setProperty(propertyName, value);
    }

    static QObject *findAccessible(QObject *object, QAccessible::Role role = QAccessible::NoRole)
    {
        while (object) {
            QObject *att = QQuickAccessibleAttached::attachedProperties(object);
            if (att && (role == QAccessible::NoRole || att->property("role").toInt() == role)) {
                break;
            }
            object = object->parent();
        }
        return object;
    }

public Q_SLOTS:
    void valueChanged() {
        QAccessibleValueChangeEvent ev(parent(), parent()->property("value"));
        QAccessible::updateAccessibility(&ev);
    }
    void cursorPositionChanged() {
        QAccessibleTextCursorEvent ev(parent(), parent()->property("cursorPosition").toInt());
        QAccessible::updateAccessibility(&ev);
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

QML_DECLARE_TYPE(QQuickAccessibleAttached)
QML_DECLARE_TYPEINFO(QQuickAccessibleAttached, QML_HAS_ATTACHED_PROPERTIES)

#endif // QT_NO_ACCESSIBILITY

#endif
