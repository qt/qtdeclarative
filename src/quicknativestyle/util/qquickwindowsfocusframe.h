// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKWINDOWSFOCUSFRAME_H
#define QQUICKWINDOWSFOCUSFRAME_H

#include "qquickfocusframe.h"

QT_BEGIN_NAMESPACE

class QQuickWindowsFocusFrame : public QQuickFocusFrame
{
    Q_OBJECT

public:
    QQuickWindowsFocusFrame() = default;

private:
    virtual QQuickItem *createFocusFrame(QQmlContext *context) override;
};

QT_END_NAMESPACE

#endif // QQUICKWINDOWSFOCUSFRAME_H
