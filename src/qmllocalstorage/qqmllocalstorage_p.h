/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
******************************************************************************/

#ifndef QQMLLOCALSTORAGE_P_H
#define QQMLLOCALSTORAGE_P_H

#include "qqmllocalstorageglobal_p.h"

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

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>
#include <QtQml/private/qv4engine_p.h>

QT_BEGIN_NAMESPACE

class Q_QMLLOCALSTORAGE_PRIVATE_EXPORT QQmlLocalStorage : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LocalStorage)
    QML_ADDED_IN_VERSION(2, 0)
    QML_SINGLETON

public:
    QQmlLocalStorage(QObject *parent = nullptr) : QObject(parent) {}
    ~QQmlLocalStorage() override = default;

    Q_INVOKABLE void openDatabaseSync(QQmlV4Function* args);
};

QT_END_NAMESPACE

#endif // QQMLLOCALSTORAGE_P_H
