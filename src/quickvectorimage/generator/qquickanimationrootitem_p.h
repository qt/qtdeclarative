// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKANIMATIONROOTITEM_P_H
#define QQUICKANIMATIONROOTITEM_P_H

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

#include <QtQuickVectorImageGenerator/qtquickvectorimagegeneratorexports.h>

#include <QtCore/qlist.h>
#include <QtCore/qpointer.h>
#include <QtCore/qvariant.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickanimation_p.h>

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORIMAGEGENERATOR_EXPORT QQuickAnimationRootItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopsChanged)
    Q_PROPERTY(QVariantMap markers READ markers WRITE setMarkersData NOTIFY markersChanged)
    Q_PROPERTY(qreal startFrame READ startFrame WRITE setStartFrame NOTIFY startFrameChanged)
    Q_PROPERTY(qreal endFrame READ endFrame WRITE setEndFrame NOTIFY endFrameChanged)
    Q_PROPERTY(qreal frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged)
    Q_PROPERTY(
            qreal frameCounter READ frameCounter WRITE setFrameCounter NOTIFY frameCounterChanged)
public:
    explicit QQuickAnimationRootItem(QQuickItem *parent = nullptr);

    bool paused() const;
    void setPaused(bool paused);

    int loops() const;
    void setLoops(int loops);

    QVariantMap markers() const;
    void setMarkersData(const QVariantMap &markers);

    qreal startFrame() const;
    void setStartFrame(qreal startFrame);

    qreal endFrame() const;
    void setEndFrame(qreal endFrame);

    qreal frameRate() const;
    void setFrameRate(qreal frameRate);

    qreal frameCounter() const;
    void setFrameCounter(qreal frameCounter);

    Q_INVOKABLE void restart();

    void addMasterAnimation(QQuickAbstractAnimation *anim);

Q_SIGNALS:
    void pausedChanged();
    void loopsChanged();
    void markersChanged();
    void startFrameChanged();
    void endFrameChanged();
    void frameRateChanged();
    void frameCounterChanged();

private:
    bool m_paused = false;
    int m_loops = 1;
    QVariantMap m_markers;
    qreal m_startFrame = 0;
    qreal m_endFrame = 0;
    qreal m_frameRate = 0;
    qreal m_frameCounter = 0;
    QList<QPointer<QQuickAbstractAnimation>> m_masterAnimations;
};

QT_END_NAMESPACE

#endif // QQUICKANIMATIONROOTITEM_P_H
