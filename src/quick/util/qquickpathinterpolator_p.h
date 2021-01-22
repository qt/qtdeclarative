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

#ifndef QQUICKPATHINTERPOLATOR_P_H
#define QQUICKPATHINTERPOLATOR_P_H

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

QT_REQUIRE_CONFIG(quick_path);

#include <qqml.h>
#include <QObject>

QT_BEGIN_NAMESPACE

class QQuickPath;
class Q_AUTOTEST_EXPORT QQuickPathInterpolator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickPath *path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(qreal x READ x NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y NOTIFY yChanged)
    Q_PROPERTY(qreal angle READ angle NOTIFY angleChanged)
    QML_NAMED_ELEMENT(PathInterpolator)
public:
    explicit QQuickPathInterpolator(QObject *parent = nullptr);

    QQuickPath *path() const;
    void setPath(QQuickPath *path);

    qreal progress() const;
    void setProgress(qreal progress);

    qreal x() const;
    qreal y() const;
    qreal angle() const;

Q_SIGNALS:
    void pathChanged();
    void progressChanged();
    void xChanged();
    void yChanged();
    void angleChanged();

private Q_SLOTS:
    void _q_pathUpdated();

private:
    QQuickPath *_path;
    qreal _x;
    qreal _y;
    qreal _angle;
    qreal _progress;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPathInterpolator)

#endif // QQUICKPATHINTERPOLATOR_P_H
