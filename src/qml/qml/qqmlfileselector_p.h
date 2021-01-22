/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
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

#ifndef QQMLFILESELECTOR_P_H
#define QQMLFILESELECTOR_P_H

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

#include "qqmlfileselector.h"
#include <QSet>
#include <QQmlAbstractUrlInterceptor>
#include <private/qobject_p.h>
#include <private/qtqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class QFileSelector;
class QQmlFileSelectorInterceptor;
class Q_QML_PRIVATE_EXPORT QQmlFileSelectorPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlFileSelector)
public:
    QQmlFileSelectorPrivate();
    ~QQmlFileSelectorPrivate();

    QFileSelector* selector;
    QPointer<QQmlEngine> engine;
    bool ownSelector;
    QScopedPointer<QQmlFileSelectorInterceptor> myInstance;
};

class Q_QML_PRIVATE_EXPORT QQmlFileSelectorInterceptor : public QQmlAbstractUrlInterceptor
{
public:
    QQmlFileSelectorInterceptor(QQmlFileSelectorPrivate* pd);
    QQmlFileSelectorPrivate* d;
protected:
    QUrl intercept(const QUrl &path, DataType type) override;
};

QT_END_NAMESPACE

#endif
