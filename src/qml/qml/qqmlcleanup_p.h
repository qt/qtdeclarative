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

#ifndef QQMLCLEANUP_P_H
#define QQMLCLEANUP_P_H

#include <private/qtqmlglobal_p.h>

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

QT_BEGIN_NAMESPACE

class QQmlEngine;

class Q_QML_PRIVATE_EXPORT QQmlCleanup
{
public:
    QQmlCleanup();
    QQmlCleanup(QQmlEngine *);
    virtual ~QQmlCleanup();

    bool hasEngine() const { return prev != nullptr; }
    void addToEngine(QQmlEngine *);
protected:
    virtual void clear() = 0;

private:
    friend class QQmlEnginePrivate;
    QQmlCleanup **prev;
    QQmlCleanup  *next;

    // Only used for asserts
    QQmlEngine *engine;
};

QT_END_NAMESPACE

#endif // QQMLCLEANUP_P_H

