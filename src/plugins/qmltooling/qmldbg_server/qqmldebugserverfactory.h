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

#ifndef QQMLDEBUGSERVERFACTORY_H
#define QQMLDEBUGSERVERFACTORY_H

#include <private/qqmldebugconnector_p.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// ementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QQmlDebugServerFactory : public QQmlDebugConnectorFactory
{
    Q_OBJECT

    // The interface for the code that loads this thing is QQmlDebugConnector.
    // QQmlDebugServer is for connection plugins.
    Q_PLUGIN_METADATA(IID QQmlDebugConnectorFactory_iid FILE "qqmldebugserver.json")
public:
    QQmlDebugConnector *create(const QString &key) override;
};

QT_END_NAMESPACE

#endif // QQMLDEBUGSERVERFACTORY_H
