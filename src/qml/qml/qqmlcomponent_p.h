// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLCOMPONENT_P_H
#define QQMLCOMPONENT_P_H

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

#include "qqmlcomponent.h"

#include "qqmlengine_p.h"
#include "qqmltypeloader_p.h"
#include "qqmlvme_p.h"
#include "qqmlerror.h"
#include "qqml.h"
#include <private/qqmlobjectcreator_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmlguardedcontextdata_p.h>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class QQmlComponent;
class QQmlEngine;

class QQmlComponentAttached;
class Q_QML_PRIVATE_EXPORT QQmlComponentPrivate : public QObjectPrivate, public QQmlTypeData::TypeDataCallback
{
    Q_DECLARE_PUBLIC(QQmlComponent)

public:
    QQmlComponentPrivate()
        : progress(0.), start(-1), engine(nullptr) {}

    void loadUrl(const QUrl &newUrl, QQmlComponent::CompilationMode mode = QQmlComponent::PreferSynchronous);

    QObject *beginCreate(QQmlRefPointer<QQmlContextData>);
    void completeCreate();
    void initializeObjectWithInitialProperties(QV4::QmlContext *qmlContext, const QV4::Value &valuemap, QObject *toCreate, RequiredProperties &requiredProperties);
    static void setInitialProperties(QV4::ExecutionEngine *engine, QV4::QmlContext *qmlContext, const QV4::Value &o, const QV4::Value &v, RequiredProperties &requiredProperties, QObject *createdComponent);
    static QQmlError unsetRequiredPropertyToQQmlError(const RequiredPropertyInfo &unsetRequiredProperty);

    virtual void incubateObject(
            QQmlIncubator *incubationTask,
            QQmlComponent *component,
            QQmlEngine *engine,
            const QQmlRefPointer<QQmlContextData> &context,
            const QQmlRefPointer<QQmlContextData> &forContext);

    QQmlRefPointer<QQmlTypeData> typeData;
    void typeDataReady(QQmlTypeData *) override;
    void typeDataProgress(QQmlTypeData *, qreal) override;

    void fromTypeData(const QQmlRefPointer<QQmlTypeData> &data);

    QUrl url;
    qreal progress;

    int start;
    RequiredProperties& requiredProperties();
    bool hadTopLevelRequiredProperties() const;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;

    struct ConstructionState {
        std::unique_ptr<QQmlObjectCreator> creator;
        QList<QQmlError> errors;
        bool completePending = false;
    };
    ConstructionState state;

    using DeferredState = std::vector<ConstructionState>;
    static void beginDeferred(QQmlEnginePrivate *enginePriv, QObject *object, DeferredState* deferredState);
    static void completeDeferred(QQmlEnginePrivate *enginePriv, DeferredState *deferredState);

    static void complete(QQmlEnginePrivate *enginePriv, ConstructionState *state);
    static QQmlProperty removePropertyFromRequired(
            QObject *createdComponent, const QString &name, RequiredProperties &requiredProperties,
            QQmlEngine *engine, bool *wasInRequiredProperties = nullptr);

    QQmlEngine *engine;
    QQmlGuardedContextData creationContext;

    void clear();

    static QQmlComponentPrivate *get(QQmlComponent *c) {
        return static_cast<QQmlComponentPrivate *>(QObjectPrivate::get(c));
    }

    QObject *doBeginCreate(QQmlComponent *q, QQmlContext *context);
    bool setInitialProperty(QObject *component, const QString &name, const QVariant& value);

    bool isBound() const {
        return compilationUnit->unitData()->flags & QV4::CompiledData::Unit::ComponentsBound;
    }
};

QT_END_NAMESPACE

#endif // QQMLCOMPONENT_P_H
