// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

class Q_QUICKCONTROLS2IMPL_PRIVATE_EXPORT QQuickAttachedObject : public QObject
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
