// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLBIND_H
#define QQMLBIND_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtqmlglobal_p.h>

#include <QtQml/qqml.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQmlBindPrivate;
class Q_QML_PRIVATE_EXPORT QQmlBind : public QObject, public QQmlPropertyValueSource, public QQmlParserStatus
{
public:
    enum RestorationMode {
        RestoreNone    = 0x0,
        RestoreBinding = 0x1,
        RestoreValue   = 0x2,
        RestoreBindingOrValue = RestoreBinding | RestoreValue
    };

private:
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlBind)
    Q_INTERFACES(QQmlParserStatus)
    Q_INTERFACES(QQmlPropertyValueSource)
    Q_PROPERTY(QObject *target READ object WRITE setObject)
    Q_PROPERTY(QString property READ property WRITE setProperty)
    Q_PROPERTY(QVariant value READ value WRITE setValue)
    Q_PROPERTY(bool when READ when WRITE setWhen)
    Q_PROPERTY(bool delayed READ delayed WRITE setDelayed REVISION(2, 8))
    Q_PROPERTY(RestorationMode restoreMode READ restoreMode WRITE setRestoreMode
               NOTIFY restoreModeChanged REVISION(2, 14))
    Q_ENUM(RestorationMode)
    QML_NAMED_ELEMENT(Binding)
    QML_ADDED_IN_VERSION(2, 0)
    Q_CLASSINFO("ImmediatePropertyNames", "objectName,target,property,value,when,delayed,restoreMode");

public:
    QQmlBind(QObject *parent=nullptr);

    bool when() const;
    void setWhen(bool);

    QObject *object();
    void setObject(QObject *);

    QString property() const;
    void setProperty(const QString &);

    QVariant value() const;
    void setValue(const QVariant &);

    bool delayed() const;
    void setDelayed(bool);

    RestorationMode restoreMode() const;
    void setRestoreMode(RestorationMode);

Q_SIGNALS:
    void restoreModeChanged();

protected:
    void setTarget(const QQmlProperty &) override;
    void classBegin() override;
    void componentComplete() override;

private:
    void prepareEval();
    void eval();

private Q_SLOTS:
    void targetValueChanged();
};

QT_END_NAMESPACE

#endif
