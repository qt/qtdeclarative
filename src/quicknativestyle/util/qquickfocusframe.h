// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKFOCUSFRAME_H
#define QQUICKFOCUSFRAME_H

#include <QtQuick/qquickitem.h>
#include "qquickstyleitem.h"

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcFocusFrame)

struct QQuickFocusFrameDescription {
    QQuickItem *target;
    QQuickStyleMargins margins;
    const qreal radius = 3;
    bool isValid() const { return target != nullptr; }
    static QQuickFocusFrameDescription Invalid;
};

class QQuickFocusFrame : public QObject
{
    Q_OBJECT

public:
    QQuickFocusFrame();

private:
    static QScopedPointer<QQuickItem> m_focusFrame;

    virtual QQuickItem *createFocusFrame(QQmlContext *context) = 0;
    void moveToItem(QQuickItem *item);
    QQuickFocusFrameDescription getDescriptionForItem(QQuickItem *focusItem) const;
};

QT_END_NAMESPACE

#endif // QQUICKFOCUSFRAME_H
