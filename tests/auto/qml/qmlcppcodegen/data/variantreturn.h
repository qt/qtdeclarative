// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef VARIANTERETURN_H
#define VARIANTERETURN_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqmlregistration.h>

#include "weathermoduleurl.h"

class DirectBindable : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(WeatherModelUrl x READ x WRITE setX NOTIFY xChanged BINDABLE bindableX)

public:
    explicit DirectBindable(QObject *parent = nullptr) : QObject(parent) {}

    WeatherModelUrl x() const { return m_x.value(); }
    void setX(const WeatherModelUrl& newX) { m_x.setValue(newX);}
    QBindable<WeatherModelUrl> bindableX() { return QBindable<WeatherModelUrl>(&m_x); }

Q_SIGNALS:
    void xChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(DirectBindable, WeatherModelUrl, m_x, &DirectBindable::xChanged)
};

class IndirectBindable : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(WeatherModelUrl y READ y WRITE setY NOTIFY yChanged BINDABLE bindableY)
    Q_PROPERTY(int z READ z NOTIFY zChanged)

public:
    explicit IndirectBindable(QObject *parent = nullptr) : QObject(parent) {
        m_z.setBinding([this]()->int {
            return m_y.value().timeIndex() * 2;
        });
    }

    WeatherModelUrl y() const { return m_y.value(); }
    void setY(const WeatherModelUrl& newY) { m_y.setValue(newY); }
    QBindable<WeatherModelUrl> bindableY() { return QBindable<WeatherModelUrl>(&m_y); }

    int z() const { return m_z.value(); }
    QBindable<int> bindableZ() const { return QBindable<int>(&m_z); }

Q_SIGNALS:
    void yChanged();
    void zChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(IndirectBindable, WeatherModelUrl, m_y, &IndirectBindable::yChanged)
    Q_OBJECT_BINDABLE_PROPERTY(IndirectBindable, int, m_z, &IndirectBindable::zChanged)
};

#endif // VARIANTRETURN_H
