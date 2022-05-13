// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "capturedevent.h"

#include <QMetaEnum>
#include <QMouseEvent>

namespace {
    static inline bool isMouseEvent(const QEvent &event)
    {
        return event.type() >= QEvent::MouseButtonPress && event.type() <= QEvent::MouseMove;
    }
}

CapturedEvent::CapturedEvent()
{
}

CapturedEvent::CapturedEvent(const QEvent &event, int delay)
{
    setEvent(event);
    setDelay(delay);
}

void CapturedEvent::setEvent(const QEvent &event)
{
    mType = event.type();

    if (isMouseEvent(event)) {
        const QMouseEvent *mouseEvent = static_cast<const QMouseEvent*>(&event);
        mPos = mouseEvent->pos();
        mMouseButton = mouseEvent->button();
    }
}

QEvent::Type CapturedEvent::type() const
{
    return mType;
}

int CapturedEvent::delay() const
{
    return mDelay;
}

void CapturedEvent::setDelay(int delay)
{
    mDelay = delay;

    mCppCommand.clear();

    // We generate the C++ command here instead of when the event is captured,
    // because events() might trim some events, causing the delay of some events to change.
    // If we did it earlier, the events wouldn't have correct delays.
    if (mType == QEvent::MouseMove) {
        mCppCommand = QString::fromLatin1("QTest::mouseMove(&view, QPoint(%1, %2), %3);")
            .arg(mPos.x())
            .arg(mPos.y())
            .arg(mDelay);

    } else if (mType >= QEvent::MouseButtonPress && mType <= QEvent::MouseButtonDblClick) {
        QString eventTestFunctionName = (mType == QEvent::MouseButtonPress
            ? "mousePress" : (mType == QEvent::MouseButtonRelease
            ? "mouseRelease" : "mouseDClick"));
        QString buttonStr = QMetaEnum::fromType<Qt::MouseButtons>().valueToKey(mMouseButton);
        mCppCommand = QString::fromLatin1("QTest::%1(&view, Qt::%2, Qt::NoModifier, QPoint(%3, %4), %5);")
            .arg(eventTestFunctionName)
            .arg(buttonStr)
            .arg(mPos.x())
            .arg(mPos.y())
            .arg(mDelay);
    }
}

QString CapturedEvent::cppCommand() const
{
    return mCppCommand;
}

