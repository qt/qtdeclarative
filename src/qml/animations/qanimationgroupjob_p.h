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

#ifndef QANIMATIONGROUPJOB_P_H
#define QANIMATIONGROUPJOB_P_H

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

#include "private/qabstractanimationjob_p.h"
#include <QtCore/qdebug.h>

QT_REQUIRE_CONFIG(qml_animation);

QT_BEGIN_NAMESPACE

class Q_QML_PRIVATE_EXPORT QAnimationGroupJob : public QAbstractAnimationJob
{
    Q_DISABLE_COPY(QAnimationGroupJob)
public:
    QAnimationGroupJob();
    ~QAnimationGroupJob() override;

    void appendAnimation(QAbstractAnimationJob *animation);
    void prependAnimation(QAbstractAnimationJob *animation);
    void removeAnimation(QAbstractAnimationJob *animation);

    QAbstractAnimationJob *firstChild() const { return m_firstChild; }
    QAbstractAnimationJob *lastChild() const { return m_lastChild; }

    virtual void clear();

    //called by QAbstractAnimationJob
    virtual void uncontrolledAnimationFinished(QAbstractAnimationJob *animation);
protected:
    void topLevelAnimationLoopChanged() override;

    virtual void animationInserted(QAbstractAnimationJob*) { }
    virtual void animationRemoved(QAbstractAnimationJob*, QAbstractAnimationJob*, QAbstractAnimationJob*);

    //TODO: confirm location of these (should any be moved into QAbstractAnimationJob?)
    void resetUncontrolledAnimationsFinishTime();
    void resetUncontrolledAnimationFinishTime(QAbstractAnimationJob *anim);
    int uncontrolledAnimationFinishTime(QAbstractAnimationJob *anim) const { return anim->m_uncontrolledFinishTime; }
    void setUncontrolledAnimationFinishTime(QAbstractAnimationJob *anim, int time);

    void debugChildren(QDebug d) const;

private:
    void ungroupChild(QAbstractAnimationJob *animation);
    void handleAnimationRemoved(QAbstractAnimationJob *animation);

    //definition
    QAbstractAnimationJob *m_firstChild = nullptr;
    QAbstractAnimationJob *m_lastChild = nullptr;
};

QT_END_NAMESPACE

#endif //QANIMATIONGROUPJOB_P_H
