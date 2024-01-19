// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RESETTABLE_H
#define RESETTABLE_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class ResettableProperty : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Resettable)
    Q_PROPERTY(qreal value READ value WRITE setValue RESET resetValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(qreal shadowable READ shadowable CONSTANT)

public:
    explicit ResettableProperty(QObject *parent = nullptr) : QObject(parent) {}
    qreal value() const { return m_value; }
    qreal shadowable() const { return 25; }

public slots:
    void resetValue() { setValue(0); }
    void setValue(qreal value)
    {
        if (m_value == value)
            return;
        m_value = value;
        emit valueChanged();
    }

signals:
    void valueChanged();

private:
    qreal m_value = 0;
};

#endif // RESETTABLE_H
