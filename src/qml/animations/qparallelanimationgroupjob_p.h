/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QPARALLELANIMATIONGROUPJOB_P_H
#define QPARALLELANIMATIONGROUPJOB_P_H

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

#include "private/qanimationgroupjob_p.h"

QT_REQUIRE_CONFIG(qml_animation);

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QParallelAnimationGroupJob : public QAnimationGroupJob
{
    Q_DISABLE_COPY(QParallelAnimationGroupJob)
public:
    QParallelAnimationGroupJob();
    ~QParallelAnimationGroupJob();

    int duration() const override;

protected:
    void updateCurrentTime(int currentTime) override;
    void updateState(QAbstractAnimationJob::State newState, QAbstractAnimationJob::State oldState) override;
    void updateDirection(QAbstractAnimationJob::Direction direction) override;
    void uncontrolledAnimationFinished(QAbstractAnimationJob *animation) override;
    void debugAnimation(QDebug d) const override;

private:
    bool shouldAnimationStart(QAbstractAnimationJob *animation, bool startIfAtEnd) const;
    void applyGroupState(QAbstractAnimationJob *animation);

    //state
    int m_previousLoop = 0;
    int m_previousCurrentTime = 0;
};

QT_END_NAMESPACE

#endif // QPARALLELANIMATIONGROUPJOB_P_H
