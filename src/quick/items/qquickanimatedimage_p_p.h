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

#ifndef QQUICKANIMATEDIMAGE_P_P_H
#define QQUICKANIMATEDIMAGE_P_P_H

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

#include <QtQuick/qtquickglobal.h>

QT_REQUIRE_CONFIG(quick_animatedimage);

#include "qquickimage_p_p.h"

QT_BEGIN_NAMESPACE

class QMovie;
#if QT_CONFIG(qml_network)
class QNetworkReply;
#endif

class QQuickAnimatedImagePrivate : public QQuickImagePrivate
{
    Q_DECLARE_PUBLIC(QQuickAnimatedImage)

public:
    QQuickAnimatedImagePrivate()
      : playing(true), paused(false), oldPlaying(false), padding(0)
      , presetCurrentFrame(0), speed(1.0), currentSourceSize(0, 0), movie(nullptr)
#if QT_CONFIG(qml_network)
      , reply(nullptr), redirectCount(0)
#endif
    {
    }

    QQuickPixmap *infoForCurrentFrame(QQmlEngine *engine);
    void setMovie(QMovie *movie);

    bool playing : 1;
    bool paused : 1;
    bool oldPlaying : 1;
    unsigned padding: 29;
    int presetCurrentFrame;
    qreal speed;
    QSize currentSourceSize;
    QMovie *movie;
#if QT_CONFIG(qml_network)
    QNetworkReply *reply;
    int redirectCount;
#endif
    QMap<int, QQuickPixmap *> frameMap;
};

QT_END_NAMESPACE

#endif // QQUICKANIMATEDIMAGE_P_P_H
