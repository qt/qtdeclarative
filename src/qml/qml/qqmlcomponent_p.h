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
#include "qqmlerror.h"
#include <private/qqmlobjectcreator_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmlguardedcontextdata_p.h>

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/qtclasshelpermacros.h>

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
    void initializeObjectWithInitialProperties(QV4::QmlContext *qmlContext, const QV4::Value &valuemap, QObject *toCreate, RequiredProperties *requiredProperties);
    static void setInitialProperties(
        QV4::ExecutionEngine *engine, QV4::QmlContext *qmlContext, const QV4::Value &o,
        const QV4::Value &v, RequiredProperties *requiredProperties, QObject *createdComponent,
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
    std::unique_ptr<QString> inlineComponentName;

    /* points to the sub-object in a QML file that should be instantiated
       used create instances of QtQml's Component type and indirectly for inline components */
    int start;

    bool hadTopLevelRequiredProperties() const;
    // TODO: merge compilation unit and type
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    QQmlType loadedType;

    struct AnnotatedQmlError
    {
        AnnotatedQmlError() = default;

        AnnotatedQmlError(QQmlError error)
            : error(std::move(error))
        {
        }


        AnnotatedQmlError(QQmlError error, bool transient)
            : error(std::move(error)), isTransient(transient)
        {
        }
        QQmlError error;
        bool isTransient = false; // tells if the error is temporary (e.g. unset required property)
    };

    struct ConstructionState {
        ConstructionState() = default;
        inline ~ConstructionState();
        Q_DISABLE_COPY(ConstructionState)
        inline ConstructionState(ConstructionState &&other) noexcept;

        void swap(ConstructionState &other)
        {
            m_creatorOrRequiredProperties.swap(other.m_creatorOrRequiredProperties);
        }

        QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QQmlComponentPrivate::ConstructionState);

        inline void ensureRequiredPropertyStorage();
        inline RequiredProperties *requiredProperties();
        inline void addPendingRequiredProperty(
                const QObject *object, const QQmlPropertyData *propData,
                const RequiredPropertyInfo &info);
        inline bool hasUnsetRequiredProperties() const;
        inline void clearRequiredProperties();

        inline void appendErrors(const QList<QQmlError> &qmlErrors);
        inline void appendCreatorErrors();

        inline QQmlObjectCreator *creator();
        inline const QQmlObjectCreator *creator() const;
        inline void clear();
        inline bool hasCreator() const;
        inline QQmlObjectCreator *initCreator(QQmlRefPointer<QQmlContextData> parentContext,
                         const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit,
                         const QQmlRefPointer<QQmlContextData> &creationContext);

        QList<AnnotatedQmlError> errors;
        inline bool isCompletePending() const;
        inline void setCompletePending(bool isPending);

        private:
        QBiPointer<QQmlObjectCreator, RequiredProperties> m_creatorOrRequiredProperties;
    };
    ConstructionState state;

    using DeferredState = std::vector<ConstructionState>;
    static void beginDeferred(QQmlEnginePrivate *enginePriv, QObject *object, DeferredState* deferredState);
    static void completeDeferred(QQmlEnginePrivate *enginePriv, DeferredState *deferredState);

    static void complete(QQmlEnginePrivate *enginePriv, ConstructionState *state);
    static QQmlProperty removePropertyFromRequired(QObject *createdComponent, const QString &name, RequiredProperties *requiredProperties,
            QQmlEngine *engine, bool *wasInRequiredProperties = nullptr);

    QQmlEngine *engine;
    QQmlGuardedContextData creationContext;

    void clear();

    static QQmlComponentPrivate *get(QQmlComponent *c) {
        return static_cast<QQmlComponentPrivate *>(QObjectPrivate::get(c));
    }

    QObject *doBeginCreate(QQmlComponent *q, QQmlContext *context);
    bool setInitialProperty(QObject *component, const QString &name, const QVariant& value);

    enum CreateBehavior {
        CreateDefault,
        CreateWarnAboutRequiredProperties,
    };
    QObject *createWithProperties(QObject *parent, const QVariantMap &properties,
                                  QQmlContext *context, CreateBehavior behavior = CreateDefault);

    bool isBound() const { return compilationUnit && (compilationUnit->componentsAreBound()); }
    LoadHelper::ResolveTypeResult prepareLoadFromModule(QAnyStringView uri,
                                                        QAnyStringView typeName);
    void completeLoadFromModule(QAnyStringView uri, QAnyStringView typeName, QQmlType type,
                                LoadHelper::ResolveTypeResult::Status moduleStatus,
                                QQmlComponent::CompilationMode mode = QQmlComponent::PreferSynchronous);
};

QQmlComponentPrivate::ConstructionState::~ConstructionState()
{
    if (m_creatorOrRequiredProperties.isT1())
        delete m_creatorOrRequiredProperties.asT1();
    else
        delete m_creatorOrRequiredProperties.asT2();
}

QQmlComponentPrivate::ConstructionState::ConstructionState(ConstructionState &&other) noexcept
{
    errors = std::move(other.errors);
    m_creatorOrRequiredProperties = std::exchange(other.m_creatorOrRequiredProperties, {});
}

/*!
   \internal A list of pending required properties that need
   to be set in order for object construction to be successful.
 */
inline RequiredProperties *QQmlComponentPrivate::ConstructionState::requiredProperties() {
    if (m_creatorOrRequiredProperties.isNull())
        return nullptr;
    else if (m_creatorOrRequiredProperties.isT1())
        return m_creatorOrRequiredProperties.asT1()->requiredProperties();
    else
        return m_creatorOrRequiredProperties.asT2();
}

inline void QQmlComponentPrivate::ConstructionState::addPendingRequiredProperty(
        const QObject *object, const QQmlPropertyData *propData, const RequiredPropertyInfo &info)
{
    Q_ASSERT(requiredProperties());
    requiredProperties()->insert({object, propData}, info);
}

inline bool QQmlComponentPrivate::ConstructionState::hasUnsetRequiredProperties() const {
    auto properties = const_cast<ConstructionState *>(this)->requiredProperties();
    return properties && !properties->isEmpty();
}

inline void QQmlComponentPrivate::ConstructionState::clearRequiredProperties()
{
    if (auto reqProps = requiredProperties())
        reqProps->clear();
}

inline void QQmlComponentPrivate::ConstructionState::appendErrors(const QList<QQmlError> &qmlErrors)
{
    for (const QQmlError &e : qmlErrors)
        errors.emplaceBack(e);
}

//! \internal Moves errors from creator into construction state itself
inline void QQmlComponentPrivate::ConstructionState::appendCreatorErrors()
{
    if (!hasCreator())
        return;
    auto creatorErrorCount = creator()->errors.size();
    if (creatorErrorCount == 0)
        return;
    auto existingErrorCount = errors.size();
    errors.resize(existingErrorCount + creatorErrorCount);
    for (qsizetype i = 0; i < creatorErrorCount; ++i)
        errors[existingErrorCount + i] = AnnotatedQmlError { std::move(creator()->errors[i]) };
    creator()->errors.clear();
}

inline QQmlObjectCreator *QQmlComponentPrivate::ConstructionState::creator()
{
    if (m_creatorOrRequiredProperties.isT1())
        return m_creatorOrRequiredProperties.asT1();
    return nullptr;
}

inline const QQmlObjectCreator *QQmlComponentPrivate::ConstructionState::creator() const
{
    if (m_creatorOrRequiredProperties.isT1())
        return m_creatorOrRequiredProperties.asT1();
    return nullptr;
}

inline bool QQmlComponentPrivate::ConstructionState::hasCreator() const
{
    return creator() != nullptr;
}

inline void QQmlComponentPrivate::ConstructionState::clear()
{
    if (m_creatorOrRequiredProperties.isT1()) {
        delete m_creatorOrRequiredProperties.asT1();
        m_creatorOrRequiredProperties = static_cast<QQmlObjectCreator *>(nullptr);
    }
}

inline QQmlObjectCreator *QQmlComponentPrivate::ConstructionState::initCreator(QQmlRefPointer<QQmlContextData> parentContext, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QQmlRefPointer<QQmlContextData> &creationContext)
{
    if (m_creatorOrRequiredProperties.isT1())
        delete m_creatorOrRequiredProperties.asT1();
    else
        delete m_creatorOrRequiredProperties.asT2();
    m_creatorOrRequiredProperties = new QQmlObjectCreator(
                        std::move(parentContext), compilationUnit,
                        creationContext);
    return m_creatorOrRequiredProperties.asT1();
}

inline bool QQmlComponentPrivate::ConstructionState::isCompletePending() const
{
    return m_creatorOrRequiredProperties.flag();
}

inline void QQmlComponentPrivate::ConstructionState::setCompletePending(bool isPending)
{
    m_creatorOrRequiredProperties.setFlagValue(isPending);
}

/*!
    \internal
    This is meant to be used in the context of QQmlComponent::loadFromModule,
    when dealing with a C++ type. In that case, we do not have a creator,
    and need a separate storage for required properties.
 */
inline void QQmlComponentPrivate::ConstructionState::ensureRequiredPropertyStorage()
{
    Q_ASSERT(m_creatorOrRequiredProperties.isT2() || m_creatorOrRequiredProperties.isNull());
    if (m_creatorOrRequiredProperties.isNull())
        m_creatorOrRequiredProperties = new RequiredProperties;
}

QT_END_NAMESPACE

#endif // QQMLCOMPONENT_P_H
