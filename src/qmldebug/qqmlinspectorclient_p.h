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

#ifndef QQMLINSPECTORCLIENT_P_H
#define QQMLINSPECTORCLIENT_P_H

#include <private/qqmldebugclient_p.h>

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

class QQmlInspectorClientPrivate;
class QQmlInspectorClient : public QQmlDebugClient
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQmlInspectorClient)

public:
    QQmlInspectorClient(QQmlDebugConnection *connection);

    int setInspectToolEnabled(bool enabled);
    int setShowAppOnTop(bool showOnTop);
    int setAnimationSpeed(qreal speed);
    int select(const QList<int> &objectIds);
    int createObject(const QString &qml, int parentId, const QStringList &imports,
                     const QString &filename);
    int moveObject(int childId, int newParentId);
    int destroyObject(int objectId);

signals:
    void responseReceived(int requestId, bool result);

protected:
    void messageReceived(const QByteArray &message) override;
};

QT_END_NAMESPACE

#endif // QQMLINSPECTORCLIENT_P_H
