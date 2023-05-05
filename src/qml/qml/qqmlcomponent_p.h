/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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
        : progress(0.), start(-1), engine(nullptr), creationContext(nullptr) {}

    void loadUrl(const QUrl &newUrl, QQmlComponent::CompilationMode mode = QQmlComponent::PreferSynchronous);

    QObject *beginCreate(QQmlRefPointer<QQmlContextData>);
    void completeCreate();
    void initializeObjectWithInitialProperties(QV4::QmlContext *qmlContext, const QV4::Value &valuemap, QObject *toCreate, RequiredProperties &requiredProperties);
    static void setInitialProperties(
        QV4::ExecutionEngine *engine, QV4::QmlContext *qmlContext, const QV4::Value &o,
        const QV4::Value &v, RequiredProperties &requiredProperties, QObject *createdComponent,
        QQmlObjectCreator *creator);
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
    bool hadRequiredProperties() const;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;

    struct ConstructionState {
        ConstructionState()
            : completePending(false)
        {}
        ~ConstructionState()
        {
        }

        QScopedPointer<QQmlObjectCreator> creator;
        QList<QQmlError> errors;
        bool completePending;
    };
    ConstructionState state;

    struct DeferredState {
        ~DeferredState() {
            qDeleteAll(constructionStates);
            constructionStates.clear();
        }
        QVector<ConstructionState *> constructionStates;
    };

    static void beginDeferred(QQmlEnginePrivate *enginePriv, QObject *object, DeferredState* deferredState);
    static void completeDeferred(QQmlEnginePrivate *enginePriv, DeferredState *deferredState);

    static void complete(QQmlEnginePrivate *enginePriv, ConstructionState *state);
    static QQmlProperty removePropertyFromRequired(QObject *createdComponent, const QString &name, RequiredProperties& requiredProperties, bool *wasInRequiredProperties = nullptr);

    QQmlEngine *engine;
    QQmlGuardedContextData creationContext;

    void clear();

    static QQmlComponentPrivate *get(QQmlComponent *c) {
        return static_cast<QQmlComponentPrivate *>(QObjectPrivate::get(c));
    }

    QObject *doBeginCreate(QQmlComponent *q, QQmlContext *context);
    bool setInitialProperty(QObject *component, const QString &name, const QVariant& value);
};

QT_END_NAMESPACE

#endif // QQMLCOMPONENT_P_H
