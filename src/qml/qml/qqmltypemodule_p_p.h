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

#ifndef QQMLTYPEMODULE_P_P_H
#define QQMLTYPEMODULE_P_P_H

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

#include <private/qqmltypemodule_p.h>
#include <private/qstringhash_p.h>
#include <private/qqmlmetatypedata_p.h>

#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE

class QQmlTypeModulePrivate
{
public:
    QQmlTypeModulePrivate(QString module, int majorVersion) :
        module(std::move(module)), majorVersion(majorVersion)
    {}

    const QString module;
    const int majorVersion = 0;

    // Can only ever decrease
    QAtomicInt minMinorVersion = std::numeric_limits<int>::max();

    // Can only ever increase
    QAtomicInt maxMinorVersion = 0;

    // Bool. Can only be set to 1 once.
    QAtomicInt locked = 0;

    typedef QStringHash<QList<QQmlTypePrivate *> > TypeHash;
    TypeHash typeHash;

    QMutex mutex;
};

QT_END_NAMESPACE

#endif // QQMLTYPEMODULE_P_P_H
