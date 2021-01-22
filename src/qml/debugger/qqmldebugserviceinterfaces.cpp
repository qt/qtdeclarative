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

#include "qqmldebugserviceinterfaces_p.h"

QT_BEGIN_NAMESPACE

const QString QV4DebugService::s_key = QStringLiteral("V8Debugger");
const QString QQmlEngineDebugService::s_key = QStringLiteral("QmlDebugger");
const QString QQmlInspectorService::s_key = QStringLiteral("QmlInspector");
const QString QQmlProfilerService::s_key = QStringLiteral("CanvasFrameRate");
const QString QDebugMessageService::s_key = QStringLiteral("DebugMessages");
const QString QQmlEngineControlService::s_key = QStringLiteral("EngineControl");
const QString QQmlNativeDebugService::s_key = QStringLiteral("NativeQmlDebugger");
const QString QQmlDebugTranslationService::s_key = QStringLiteral("DebugTranslation");

QT_END_NAMESPACE

#include "moc_qqmldebugserviceinterfaces_p.cpp"
