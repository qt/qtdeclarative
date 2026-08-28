// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QABSTRACTOBJECTREGISTRYREF_H
#define QABSTRACTOBJECTREGISTRYREF_H

#include <QtQmlDesignSupport/qtqmldesignsupportexports.h>

#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QAbstractObjectRegistryRefPrivate;

class Q_QMLDESIGNSUPPORT_EXPORT QAbstractObjectRegistryRef : public QObject
{
    Q_OBJECT

    Q_DECLARE_PRIVATE(QAbstractObjectRegistryRef)

    Q_PROPERTY(QString key READ key WRITE setKey NOTIFY keyChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QObject> data READ data NOTIFY dataChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "data")

    QML_NAMED_ELEMENT(AbstractObjectRegistryRef)
    QML_UNCREATABLE("AbstractObjectRegistryRef is Abstract")

public:
    ~QAbstractObjectRegistryRef() override;

    QString key() const;
    void setKey(const QString &key);

    QQmlListProperty<QObject> data();

protected:
    explicit QAbstractObjectRegistryRef(QAbstractObjectRegistryRefPrivate &dd,
                                        QObject *parent = nullptr);

    bool event(QEvent *) override;

Q_SIGNALS:
    void keyChanged(const QString &key);
    void dataChanged();
};

QT_END_NAMESPACE

#endif // QABSTRACTOBJECTREGISTRYREF_H
