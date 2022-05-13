// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef AVATAREXAMPLE_P_H
#define AVATAREXAMPLE_P_H

#include <QObject>
#include <QPixmap>
#include <qqml.h>

//![0]
// avatarExample.h
class AvatarExample : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QPixmap avatar READ avatar WRITE setAvatar NOTIFY avatarChanged)
    QML_ELEMENT

public:
    AvatarExample(QObject *parent = nullptr)
        : QObject(parent), m_value(100, 100)
    {
        m_value.fill(Qt::blue);
    }

    ~AvatarExample() {}

    QPixmap avatar() const { return m_value; }
    void setAvatar(QPixmap v) { m_value = v; emit avatarChanged(); }

signals:
    void avatarChanged();

private:
    QPixmap m_value;
};
//![0]

#endif
