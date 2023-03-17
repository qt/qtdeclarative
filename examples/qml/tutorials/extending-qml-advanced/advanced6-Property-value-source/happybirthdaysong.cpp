// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "happybirthdaysong.h"

#include <QTimer>

HappyBirthdaySong::HappyBirthdaySong(QObject *parent) : QObject(parent)
{
    auto *timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, this, &HappyBirthdaySong::advance);
    timer->start(1000);
}

void HappyBirthdaySong::setTarget(const QQmlProperty &target)
{
    m_target = target;
}

QString HappyBirthdaySong::name() const
{
    return m_name;
}

void HappyBirthdaySong::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged();
    }

    m_lyrics.clear();
    m_lyrics << "Happy birthday to you,";
    m_lyrics << "Happy birthday to you,";
    m_lyrics << "Happy birthday dear " + m_name + ",";
    m_lyrics << "Happy birthday to you!";
    m_lyrics << "";
}

void HappyBirthdaySong::advance()
{
    m_line = (m_line + 1) % m_lyrics.count();

    m_target.write(m_lyrics.at(m_line));
}
