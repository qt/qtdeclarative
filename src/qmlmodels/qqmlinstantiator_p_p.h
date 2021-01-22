/****************************************************************************
**
** Copyright (C) 2016 Research In Motion.
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

#ifndef QQMLINSTANTIATOR_P_P_H
#define QQMLINSTANTIATOR_P_P_H

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

#include "qqmlinstantiator_p.h"
#include <QObject>
#include <private/qobject_p.h>
#include <private/qqmlchangeset_p.h>
#include <private/qqmlobjectmodel_p.h>

QT_REQUIRE_CONFIG(qml_object_model);

QT_BEGIN_NAMESPACE

class Q_QMLMODELS_PRIVATE_EXPORT QQmlInstantiatorPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlInstantiator)

public:
    QQmlInstantiatorPrivate();
    ~QQmlInstantiatorPrivate();

    void clear();
    void regenerate();
#if QT_CONFIG(qml_delegate_model)
    void makeModel();
#endif
    void _q_createdItem(int, QObject *);
    void _q_modelUpdated(const QQmlChangeSet &, bool);
    QObject *modelObject(int index, bool async);

    static QQmlInstantiatorPrivate *get(QQmlInstantiator *instantiator) { return instantiator->d_func(); }
    static const QQmlInstantiatorPrivate *get(const QQmlInstantiator *instantiator) { return instantiator->d_func(); }

    bool componentComplete:1;
    bool effectiveReset:1;
    bool active:1;
    bool async:1;
#if QT_CONFIG(qml_delegate_model)
    bool ownModel:1;
#endif
    int requestedIndex;
    QVariant model;
    QQmlInstanceModel *instanceModel;
    QQmlComponent *delegate;
    QVector<QPointer<QObject> > objects;
};

QT_END_NAMESPACE

#endif // QQMLCREATOR_P_P_H
