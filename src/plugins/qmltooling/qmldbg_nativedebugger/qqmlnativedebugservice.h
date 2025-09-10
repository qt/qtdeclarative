// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQML_NATIVE_DEBUG_SERVICE_H
#define QQML_NATIVE_DEBUG_SERVICE_H

#include <private/qqmldebugconnector_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv4engine_p.h>
#include <private/qv4debugging_p.h>
#include <private/qv4script_p.h>
#include <private/qv4string_p.h>
#include <private/qv4objectiterator_p.h>
#include <private/qv4identifierhash_p.h>
#include <private/qv4runtime_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>

#include <QtCore/qjsonarray.h>

#include <qqmlengine.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
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
