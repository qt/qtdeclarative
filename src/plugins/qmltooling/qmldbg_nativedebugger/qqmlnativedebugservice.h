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

#ifndef QQML_NATIVE_DEBUG_SERVICE_H
#define QQML_NATIVE_DEBUG_SERVICE_H

#include <private/qqmldebugconnector_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv4script_p.h>
#include <private/qv4string_p.h>
#include <private/qv4objectiterator_p.h>
#include <private/qv4identifier_p.h>
#include <private/qv4runtime_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>

#include <QtCore/qjsonarray.h>

#include <qqmlengine.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QVector>
#include <QPointer>

QT_BEGIN_NAMESPACE

class NativeDebugger;
class BreakPointHandler;

class QQmlNativeDebugServiceImpl : public QQmlNativeDebugService
{
public:
    QQmlNativeDebugServiceImpl(QObject *parent);

    ~QQmlNativeDebugServiceImpl() override;

    void engineAboutToBeAdded(QJSEngine *engine) override;
    void engineAboutToBeRemoved(QJSEngine *engine) override;

    void stateAboutToBeChanged(State state) override;

    void messageReceived(const QByteArray &message) override;

    void emitAsynchronousMessageToClient(const QJsonObject &message);

private:
    friend class NativeDebugger;

    QList<QPointer<NativeDebugger> > m_debuggers;
    BreakPointHandler *m_breakHandler;
};

QT_END_NAMESPACE

#endif // QQML_NATIVE_DEBUG_SERVICE_H
