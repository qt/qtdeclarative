// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TYPEWITHSPECIALPROPERTIES_H
#define TYPEWITHSPECIALPROPERTIES_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqmlregistration.h>

class TypeWithSpecialProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int x MEMBER m_x)
    Q_PROPERTY(QString y MEMBER m_y)
    Q_PROPERTY(double z BINDABLE bindableZ READ default WRITE default)

    // for alias attributes:
    Q_PROPERTY(QString xx READ xx CONSTANT)
    Q_PROPERTY(QString xy READ xy WRITE setXy RESET resetXy NOTIFY xyChanged)

    QProperty<int> m_x;
    QProperty<double> m_z;

    QProperty<QString> m_xx;
    QProperty<QString> m_xy;

public:
    QProperty<QString> m_y;

    TypeWithSpecialProperties(QObject *parent = nullptr) : QObject(parent) { }

    QBindable<double> bindableZ() { return QBindable<double>(&m_z); }

    QString xx() const { return m_xx; }
    QString xy() const { return m_xy; }
    void setXy(const QString &xy)
    {
        m_xy = xy;
        Q_EMIT xyChanged();
    }
    void resetXy() { m_xy = QStringLiteral("reset"); }

Q_SIGNALS:
    void xyChanged();
};

#endif // TYPEWITHSPECIALPROPERTIES_H
