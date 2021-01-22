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

#ifndef DESIGNERSUPPORTITEM_H
#define DESIGNERSUPPORTITEM_H

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

#include "qquickdesignersupport_p.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QList>
#include <QByteArray>
#include <QQmlContext>
#include <QQmlListReference>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QQuickDesignerSupportItems
{
public:
    static QObject *createPrimitive(const QString &typeName, int majorNumber, int minorNumber, QQmlContext *context);
    static QObject *createComponent(const QUrl &componentUrl, QQmlContext *context);
    static void tweakObjects(QObject *object);
    static bool objectWasDeleted(QObject *object);
    static void disableNativeTextRendering(QQuickItem *item);
    static void disableTextCursor(QQuickItem *item);
    static void disableTransition(QObject *object);
    static void disableBehaivour(QObject *object);
    static void stopUnifiedTimer();
    static void registerFixResourcePathsForObjectCallBack(void (*callback)(QObject *));
};

QT_END_NAMESPACE

#endif // DESIGNERSUPPORTITEM_H
