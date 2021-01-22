/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQUICKSPRITESEQUENCE_P_P_H
#define QQUICKSPRITESEQUENCE_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_sprite);

#include "qquickitem_p.h"
#include "qquicksprite_p.h"
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE

class QQuickSpriteSequence;

class QQuickSpriteSequencePrivate :public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickSpriteSequence)
public:
    QQuickSpriteSequencePrivate()
        : m_spriteEngine(nullptr)
        , m_curFrame(0)
        , m_pleaseReset(false)
        , m_running(true)
        , m_interpolate(true)
        , m_curStateIdx(0)
    {

    }
    QList<QQuickSprite*> m_sprites;
    QQuickSpriteEngine* m_spriteEngine;
    QElapsedTimer m_timestamp;
    int m_curFrame;
    bool m_pleaseReset;
    bool m_running;
    bool m_interpolate;
    QString m_goalState;
    QString m_curState;
    int m_curStateIdx;
    QSize m_sheetSize;
};

QT_END_NAMESPACE

#endif // QQUICKSPRITESEQUENCE_P_P_H
