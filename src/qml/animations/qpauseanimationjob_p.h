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

#ifndef QPAUSEANIMATIONJOB_P_H
#define QPAUSEANIMATIONJOB_P_H

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

#include <private/qanimationgroupjob_p.h>

QT_REQUIRE_CONFIG(qml_animation);

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QPauseAnimationJob : public QAbstractAnimationJob
{
    Q_DISABLE_COPY(QPauseAnimationJob)
public:
    explicit QPauseAnimationJob(int duration = 250);
    ~QPauseAnimationJob() override;

    int duration() const override;
    void setDuration(int msecs);

protected:
    void updateCurrentTime(int) override;
    void debugAnimation(QDebug d) const override;

private:
    //definition
    int m_duration;
};

QT_END_NAMESPACE

#endif // QPAUSEANIMATIONJOB_P_H
