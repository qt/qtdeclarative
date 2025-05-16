// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQUICKMACFOCUSFRAME_H
#define QQUICKMACFOCUSFRAME_H

#include "qquickfocusframe.h"

QT_BEGIN_NAMESPACE

class QQuickMacFocusFrame : public QQuickFocusFrame
{
    Q_OBJECT

public:
    QQuickMacFocusFrame() = default;

private:
    virtual QQuickItem *createFocusFrame(QQmlContext *context) override;
};

QT_END_NAMESPACE

#endif // QQUICKMACFOCUSFRAME_H
