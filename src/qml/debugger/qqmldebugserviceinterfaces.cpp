// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldebugserviceinterfaces_p.h"

QT_BEGIN_NAMESPACE

const QString QV4DebugService::s_key = QStringLiteral("V8Debugger");
const QString QQmlEngineDebugService::s_key = QStringLiteral("QmlDebugger");
const QString QQmlInspectorService::s_key = QStringLiteral("QmlInspector");
const QString QQmlProfilerService::s_key = QStringLiteral("CanvasFrameRate");
const QString QDebugMessageService::s_key = QStringLiteral("DebugMessages");
const QString QQmlEngineControlService::s_key = QStringLiteral("EngineControl");
const QString QQmlNativeDebugService::s_key = QStringLiteral("NativeQmlDebugger");
#if QT_CONFIG(translation)
const QString QQmlDebugTranslationService::s_key = QStringLiteral("DebugTranslation");
#endif

QV4DebugService::~QV4DebugService()
    = default;
QQmlEngineDebugService::~QQmlEngineDebugService()
    = default;
QQmlInspectorService::~QQmlInspectorService()
    = default;
QQmlProfilerService::~QQmlProfilerService()
    = default;
QDebugMessageService::~QDebugMessageService()
    = default;
QQmlEngineControlService::~QQmlEngineControlService()
    = default;
QQmlNativeDebugService::~QQmlNativeDebugService()
    = default;

static QQmlDebugStatesDelegate *(*statesDelegateFactory)() = nullptr;
void QQmlEngineDebugService::setStatesDelegateFactory(QQmlDebugStatesDelegate *(*factory)())
{
    statesDelegateFactory = factory;
}

QQmlDebugStatesDelegate *QQmlEngineDebugService::createStatesDelegate()
{
    return statesDelegateFactory ? statesDelegateFactory() : nullptr;
}

#if QT_CONFIG(translation)
QQmlDebugTranslationService::~QQmlDebugTranslationService()
    = default;
#endif

QT_END_NAMESPACE

#include "moc_qqmldebugserviceinterfaces_p.cpp"
