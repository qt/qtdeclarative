// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKMACFOCUSFRAME_H
#define QQUICKMACFOCUSFRAME_H

#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquicktextedit_p.h>
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

class QQuickMacFocusFrame : public  QObject
{
    Q_OBJECT

public:
    QQuickMacFocusFrame();

private:
    static QScopedPointer<QQuickItem> m_focusFrame;

    void createFocusFrame(QQmlContext *context);
    void moveToItem(QQuickItem *item);
    QQuickFocusFrameDescription getDescriptionForItem(QQuickItem *focusItem) const;
};

QT_END_NAMESPACE

#endif // QQUICKMACFOCUSFRAME_H
