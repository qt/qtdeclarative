/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKFRAMEANIMATION_H
#define QQUICKFRAMEANIMATION_H

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

#include <qqml.h>
#include <QtCore/qobject.h>
#include <private/qtqmlglobal_p.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickFrameAnimationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickFrameAnimation : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickFrameAnimation)
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int currentFrame READ currentFrame NOTIFY currentFrameChanged)
    Q_PROPERTY(qreal frameTime READ frameTime NOTIFY frameTimeChanged)
    Q_PROPERTY(qreal smoothFrameTime READ smoothFrameTime NOTIFY smoothFrameTimeChanged)
    Q_PROPERTY(qreal elapsedTime READ elapsedTime NOTIFY elapsedTimeChanged)
    QML_NAMED_ELEMENT(FrameAnimation)
    QML_ADDED_IN_VERSION(6, 4)

public:
    QQuickFrameAnimation(QObject *parent = nullptr);

    bool isRunning() const;
    void setRunning(bool running);

    bool isPaused() const;
    void setPaused(bool paused);

    int currentFrame() const;
    qreal frameTime() const;
    qreal smoothFrameTime() const;
    qreal elapsedTime() const;

protected:
    void classBegin() override;
    void componentComplete() override;

public Q_SLOTS:
    void start();
    void stop();
    void restart();
    void pause();
    void resume();
    void reset();

Q_SIGNALS:
    void triggered();
    void runningChanged();
    void pausedChanged();
    void currentFrameChanged();
    void frameTimeChanged();
    void smoothFrameTimeChanged();
    void elapsedTimeChanged();

private:
    void setCurrentFrame(int frame);
    void setElapsedTime(qreal elapsedTime);

};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFrameAnimation)

#endif
