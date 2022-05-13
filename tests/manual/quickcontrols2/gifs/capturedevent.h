// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CAPTUREDEVENT_H
#define CAPTUREDEVENT_H

#include <QEvent>
#include <QPoint>
#include <QString>

class CapturedEvent
{
public:
    CapturedEvent();
    CapturedEvent(const QEvent &event, int delay);

    void setEvent(const QEvent &event);

    int delay() const;
    void setDelay(int delay);

    QEvent::Type type() const;

    QString cppCommand() const;

private:
    QEvent::Type mType;
    QPoint mPos;
    Qt::MouseButton mMouseButton;
    int mDelay;
    QString mCppCommand;
};

#endif // CAPTUREDEVENT_H
