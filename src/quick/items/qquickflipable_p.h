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

#ifndef QQUICKFLIPABLE_P_H
#define QQUICKFLIPABLE_P_H

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

QT_REQUIRE_CONFIG(quick_flipable);

#include "qquickitem.h"

#include <QtGui/qtransform.h>
#include <QtGui/qvector3d.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQuickFlipablePrivate;
class Q_AUTOTEST_EXPORT QQuickFlipable : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *front READ front WRITE setFront NOTIFY frontChanged)
    Q_PROPERTY(QQuickItem *back READ back WRITE setBack NOTIFY backChanged)
    Q_PROPERTY(Side side READ side NOTIFY sideChanged)
    QML_NAMED_ELEMENT(Flipable)
    //### flipAxis
    //### flipRotation
public:
    QQuickFlipable(QQuickItem *parent=nullptr);
    ~QQuickFlipable();

    QQuickItem *front() const;
    void setFront(QQuickItem *);

    QQuickItem *back();
    void setBack(QQuickItem *);

    enum Side { Front, Back };
    Q_ENUM(Side)
    Side side() const;

Q_SIGNALS:
    void frontChanged();
    void backChanged();
    void sideChanged();

protected:
    void updatePolish() override;

private Q_SLOTS:
    void retransformBack();

private:
    Q_DISABLE_COPY(QQuickFlipable)
    Q_DECLARE_PRIVATE(QQuickFlipable)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFlipable)

#endif // QQUICKFLIPABLE_P_H
