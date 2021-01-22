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

#ifndef QQUICKANIMATEDIMAGE_P_H
#define QQUICKANIMATEDIMAGE_P_H

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

QT_REQUIRE_CONFIG(quick_animatedimage);

#include "qquickimage_p.h"

QT_BEGIN_NAMESPACE

class QMovie;
class QQuickAnimatedImagePrivate;

class Q_AUTOTEST_EXPORT QQuickAnimatedImage : public QQuickImage
{
    Q_OBJECT

    Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int currentFrame READ currentFrame WRITE setCurrentFrame NOTIFY frameChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY frameCountChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged REVISION 11)

    // read-only for AnimatedImage
    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
    QML_NAMED_ELEMENT(AnimatedImage)

public:
    QQuickAnimatedImage(QQuickItem *parent=nullptr);
    ~QQuickAnimatedImage();

    bool isPlaying() const;
    void setPlaying(bool play);

    bool isPaused() const;
    void setPaused(bool pause);

    int currentFrame() const override;
    void setCurrentFrame(int frame) override;

    int frameCount() const override;

    qreal speed() const;
    void setSpeed(qreal speed);

    // Extends QQuickImage's src property
    void setSource(const QUrl&) override;
    virtual QSize sourceSize();

Q_SIGNALS:
    void playingChanged();
    void pausedChanged();
    void frameChanged();
    void currentFrameChanged();
    void frameCountChanged();
    Q_REVISION(11) void speedChanged();

private Q_SLOTS:
    void movieUpdate();
    void movieRequestFinished();
    void playingStatusChanged();
    void onCacheChanged();

protected:
    void load() override;
    void componentComplete() override;

private:
    Q_DISABLE_COPY(QQuickAnimatedImage)
    Q_DECLARE_PRIVATE(QQuickAnimatedImage)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickAnimatedImage)

#endif // QQUICKANIMATEDIMAGE_P_H
