// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PROPERTYALIASATTRIBUTES_H
#define PROPERTYALIASATTRIBUTES_H

#include <QtCore/qobject.h>
#include <QtCore/QBindable>
#include <QtQml/qqmlregistration.h>

using namespace Qt::Literals;

class TypeWithManyProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    const QString m_readOnly = u"Hello World!"_s;
    QString m_readAndWrite;
    QString m_resettable;
    QString m_unresettable;
    QString m_hasAllAttributes;
    QString m_hasAllAttributes2;

public:
    TypeWithManyProperties()
    {
        hasAllAttributesBindable().setBinding(
                [&] { return u"From the bindable: "_s + readOnly(); });
    }

    Q_PROPERTY(QString readOnly READ readOnly);
    Q_PROPERTY(QString readAndWrite READ readAndWrite WRITE setReadAndWrite);
    Q_PROPERTY(QString readAndWriteMember MEMBER m_readAndWrite);
    Q_PROPERTY(QString resettable READ resettable WRITE setReadAndWrite RESET resetResettable);
    Q_PROPERTY(QString unresettable READ unresettable WRITE setUnresettable);
    Q_PROPERTY(QString notifiable READ readAndWrite WRITE setReadAndWriteAndNotify NOTIFY
                       notifiableChanged);
    Q_PROPERTY(QString notifiableMember MEMBER m_readAndWrite NOTIFY notifiableChanged);
    Q_PROPERTY(QString latestReadAndWrite MEMBER m_readAndWrite REVISION(1, 0));
    Q_PROPERTY(QString notExisting MEMBER m_readAndWrite REVISION(6, 0));

    Q_PROPERTY(QString hasAllAttributes READ hasAllAttributes WRITE setHasAllAttributes RESET
                       resetHasAllAttributes NOTIFY hasAllAttributesChanged REVISION(1, 0)
                               BINDABLE hasAllAttributesBindable
                                       DESIGNABLE false SCRIPTABLE true STORED false USER true FINAL
                                               REQUIRED);

    Q_OBJECT_BINDABLE_PROPERTY(TypeWithManyProperties, QString, hasAllAttributesProperty);

    QBindable<QString> hasAllAttributesBindable()
    {
        return QBindable<QString>(&hasAllAttributesProperty);
    }

    Q_PROPERTY(QString hasAllAttributes2 READ hasAllAttributes2
                       DESIGNABLE true SCRIPTABLE true STORED true USER false CONSTANT);

    QString readOnly() { return m_readOnly; }

    QString readAndWrite() { return m_readAndWrite; }

    void setReadAndWrite(const QString &s) { m_readAndWrite = s; }

    void setReadAndWriteAndNotify(const QString &s)
    {
        if (s != readAndWrite()) {
            setReadAndWrite(s);
            emit notifiableChanged(s);
        }
    }

    void resetResettable() { m_resettable = u"Reset!"_s; }

    QString resettable() { return m_resettable; }
    QString unresettable() { return m_unresettable; }
    void setResettable(const QString &s) { m_resettable = s; }
    void setUnresettable(const QString &s) { m_unresettable = s; }

    QString hasAllAttributes2() { return u"Some Constant string"_s; }
    QString hasAllAttributes() { return m_hasAllAttributes; }
    void setHasAllAttributes(const QString &s) { m_hasAllAttributes = s; }
    void resetHasAllAttributes() { m_hasAllAttributes = "This value has been reset."; }

signals:
    void notifiableChanged(const QString &newValue);
    void hasAllAttributesChanged(const QString &newValue);
};

#endif // PROPERTYALIASATTRIBUTES_H
