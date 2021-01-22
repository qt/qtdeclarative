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

#ifndef QQMLCOMPONENTATTACHED_P_H
#define QQMLCOMPONENTATTACHED_P_H

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
#include <private/qtqmlglobal_p.h>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE


class Q_QML_PRIVATE_EXPORT QQmlComponentAttached : public QObject
{
    Q_OBJECT

    // Used as attached object for QQmlComponent. We want qqmlcomponentattached_p.h to be #include'd
    // when registering QQmlComponent, but we cannot #include it from qqmlcomponent.h. Therefore we
    // force an anonymous type registration here.
    QML_ANONYMOUS
public:
    QQmlComponentAttached(QObject *parent = nullptr);
    ~QQmlComponentAttached();

    void add(QQmlComponentAttached **a) {
        prev = a; next = *a; *a = this;
        if (next) next->prev = &next;
    }
    void rem() {
        if (next) next->prev = prev;
        *prev = next;
        next = nullptr; prev = nullptr;
    }
    QQmlComponentAttached **prev;
    QQmlComponentAttached *next;

Q_SIGNALS:
    void completed();
    void destruction();

private:
    friend class QQmlContextData;
};

QT_END_NAMESPACE

#endif // QQMLCOMPONENTATTACHED_P_H
