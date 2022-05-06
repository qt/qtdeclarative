/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQUICKATTACHEDOBJECT_P_H
#define QQUICKATTACHEDOBJECT_P_H

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

#include <QtQml/qqml.h>
#include <QtCore/qobject.h>
#include <QtQuickControls2Impl/private/qtquickcontrols2implglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickAttachedObjectPrivate;

class Q_QUICKCONTROLS2_PRIVATE_EXPORT QQuickAttachedObject : public QObject
{
    Q_OBJECT

public:
    explicit QQuickAttachedObject(QObject *parent = nullptr);
    ~QQuickAttachedObject();

    QList<QQuickAttachedObject *> attachedChildren() const;

    QQuickAttachedObject *attachedParent() const;
    void setAttachedParent(QQuickAttachedObject *parent);

protected:
    void init();

    virtual void attachedParentChange(QQuickAttachedObject *newParent, QQuickAttachedObject *oldParent);

private:
    Q_DISABLE_COPY(QQuickAttachedObject)
    Q_DECLARE_PRIVATE(QQuickAttachedObject)
};

QT_END_NAMESPACE

#endif // QQUICKATTACHEDOBJECT_P_H
