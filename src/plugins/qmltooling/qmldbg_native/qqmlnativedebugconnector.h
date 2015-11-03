/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLNATIVEDEBUGCONNECTOR_H
#define QQMLNATIVEDEBUGCONNECTOR_H

#include <private/qqmldebugconnector_p.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class QQmlNativeDebugConnector : public QQmlDebugConnector
{
    Q_OBJECT

public:
    QQmlNativeDebugConnector();
    ~QQmlNativeDebugConnector();

    bool blockingMode() const;
    QQmlDebugService *service(const QString &name) const;
    void addEngine(QQmlEngine *engine);
    void removeEngine(QQmlEngine *engine);
    bool addService(const QString &name, QQmlDebugService *service);
    bool removeService(const QString &name);
    bool open(const QVariantHash &configuration);
    static void setDataStreamVersion(int version);

private slots:
    void sendMessage(const QString &name, const QByteArray &message);
    void sendMessages(const QString &name, const QList<QByteArray> &messages);

private:
    void announceObjectAvailability(const QString &objectType, QObject *object, bool available);

    QVector<QQmlDebugService *> m_services;
    bool m_blockingMode;
};

class QQmlNativeDebugConnectorFactory : public QQmlDebugConnectorFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlDebugConnectorFactory_iid FILE "qqmlnativedebugconnector.json")
public:
    QQmlDebugConnector *create(const QString &key);
};

QT_END_NAMESPACE

#endif // QQMLNATIVEDEBUGCONNECTOR_H

