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

const TranslationBindingInformation TranslationBindingInformation::create(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
        const QV4::CompiledData::Binding *binding, QObject *scopeObject,
        QQmlRefPointer<QQmlContextData> ctxt)
{
    QQmlTranslation translation;
    if (binding->type() == QV4::CompiledData::Binding::Type_TranslationById) {
        const QV4::CompiledData::TranslationData data =
                compilationUnit->data->translations()[binding->value.translationDataIndex];
        const QString id = compilationUnit->stringAt(data.stringIndex);
        const int n = data.number;

        translation = QQmlTranslation(QQmlTranslation::QsTrIdData(id, n));
    } else {
        Q_ASSERT(binding->type() == QV4::CompiledData::Binding::Type_Translation);

        const QV4::CompiledData::TranslationData data =
                compilationUnit->data->translations()[binding->value.translationDataIndex];
        const QString text = compilationUnit->stringAt(data.stringIndex);
        const QString comment = compilationUnit->stringAt(data.commentIndex);
        const bool hasContext
                = data.contextIndex != QV4::CompiledData::TranslationData::NoContextIndex;
        const int n = data.number;

        translation = QQmlTranslation(
                    QQmlTranslation::QsTrData(
                        hasContext
                                ? compilationUnit->stringAt(data.contextIndex)
                                : QQmlTranslation::contextFromQmlFilename(
                                        compilationUnit->fileName()),
                        text, comment, n));
    }

    return { compilationUnit,
             scopeObject,
             ctxt,

             compilationUnit->stringAt(binding->propertyNameIndex),
             translation,

             binding->location.line(),
             binding->location.column() };
}
#endif

QT_END_NAMESPACE

#include "moc_qqmldebugserviceinterfaces_p.cpp"
