// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlfunctionfilter_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>
#include <QtQml/private/qqmlobjectcreator_p.h>
#include <QObject>
#include <QMetaMethod>

QT_BEGIN_NAMESPACE

/*!
    \qmltype FunctionFilter
    \inherits Filter
    \inqmlmodule QtQml.Models
    \since 6.10
    \preliminary
    \brief Filters data in a \l SortFilterProxyModel based on the evaluation
    of the designated 'filter' method.

    FunctionFilter allows user to define the designated 'filter' method and it
    will be evaluated to filter the data. The 'filter' method takes one
    argument and it can be defined as inline component as below:

    \qml
    SortFilterProxyModel {
        sourceModel: model
        filters: [
            FunctionFilter {
                id: functionFilter
                property int ageLimit: 20
                component RoleData: QtObject {
                    property real age
                }
                function filter(data: RoleData) : bool {
                    return (data.age <= ageLimit)
                }
            }
        ]
    }
    \endqml

    \note The user needs to explicitly invoke
    \l{SortFilterProxyModel::invalidate} whenever any external qml property
    used within the designated 'filter' method changes. This behaviour is
    subject to change in the future, like implicit invalidation and thus the
    user doesn't need to explicitly invoke
    \l{SortFilterProxyModel::invalidate}.
*/

QQmlFunctionFilter::QQmlFunctionFilter(QObject *parent)
    : QQmlFilterBase (new QQmlFunctionFilterPrivate, parent)
{
}

QQmlFunctionFilter::~QQmlFunctionFilter()
{
    Q_D(QQmlFunctionFilter);
    if (d->m_parameterData.metaType().flags() & QMetaType::PointerToQObject)
        delete d->m_parameterData.value<QObject *>();
}

void QQmlFunctionFilter::componentComplete()
{
    Q_D(QQmlFunctionFilter);
    const auto *metaObj = metaObject();
    for (int idx = metaObj->methodOffset(); idx < metaObj->methodCount(); idx++) {
        // Once we find the method signature, break the loop
        QMetaMethod method = metaObj->method(idx);
        if (method.nameView() == "filter") {
            d->m_method = method;
            break;
        }
    }

    if (!d->m_method.isValid())
        return;

    if (d->m_method.parameterCount() != 1) {
        qWarning("filter method requires a single parameter");
        return;
    }

    QQmlData *data = QQmlData::get(this);
    if (!data || !data->outerContext) {
        qWarning("filter requires a QML context");
        return;
    }

    QQmlRefPointer<QQmlContextData> context = data->outerContext;
    QQmlEngine *engine = context->engine();

    const QMetaType parameterType = d->m_method.parameterMetaType(0);
    auto cu = QQmlMetaType::obtainCompilationUnit(parameterType);
    const QQmlType parameterQmlType = QQmlMetaType::qmlType(parameterType);

    if (!parameterQmlType.isValid()) {
        qWarning("filter method parameter needs to be a QML-registered type");
        return;
    }

    // The code below creates an instance of the inline component, composite,
    // or specific C++ QObject types. The created instance, along with the
    // data, is passed as an argument to the 'filter' method, which is invoked
    // during the call to QQmlFunctionFilter::filterAcceptsRowInternal.
    // To create an instance of required component types (be it inline or
    // composite), an executable compilation unit is required, and this can be
    // obtained by looking up via metatype in the type registry
    // (QQmlMetaType::obtainCompilationUnit). Pass it through the QML engine to
    // make it executable. Further, use the executable compilation unit to run
    // an object creator and produce an instance.
    if (parameterType.flags() & QMetaType::PointerToQObject) {
        QObject *created = nullptr;
        if (parameterQmlType.isInlineComponentType()) {
            const auto executableCu = engine->handle()->executableCompilationUnit(std::move(cu));
            const QString icName = parameterQmlType.elementName();
            created = QQmlObjectCreator(context, executableCu, context, icName).create(
                    executableCu->inlineComponentId(icName), nullptr, nullptr,
                    QQmlObjectCreator::InlineComponent);
        } else if (parameterQmlType.isComposite()) {
            const auto executableCu = engine->handle()->executableCompilationUnit(std::move(cu));
            created = QQmlObjectCreator(context, executableCu, context, QString()).create();
        } else {
            created = parameterQmlType.metaObject()->newInstance();
        }

        const auto names = d->m_method.parameterNames();
        created->setObjectName(names[0]);
        d->m_parameterData = QVariant::fromValue(created);
    } else {
        d->m_parameterData = QVariant(parameterType);
    }
}

/*!
    \internal
*/
bool QQmlFunctionFilter::filterAcceptsRowInternal(int row, const QModelIndex& sourceParent, const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlFunctionFilter);
    if (!d->m_method.isValid() || !d->m_parameterData.isValid())
        return true;

    bool retVal = false;
    if (column() > -1) {
        QSortFilterProxyModelHelper::setProperties(
                &d->m_parameterData, proxyModel,
                        proxyModel->sourceModel()->index(row, column(), sourceParent));
        void *argv[] = {&retVal, d->m_parameterData.data()};
        QMetaObject::metacall(
                const_cast<QQmlFunctionFilter *>(this), QMetaObject::InvokeMetaMethod,
                d->m_method.methodIndex(), argv);
    } else {
        const int columnCount = proxyModel->sourceModel()->columnCount(sourceParent);
        for (int column = 0; column < columnCount; column++) {
            QSortFilterProxyModelHelper::setProperties(
                    &d->m_parameterData, proxyModel,
                    proxyModel->sourceModel()->index(row, column, sourceParent));
            void *argv[] = {&retVal, d->m_parameterData.data()};
            QMetaObject::metacall(
                    const_cast<QQmlFunctionFilter *>(this), QMetaObject::InvokeMetaMethod,
                    d->m_method.methodIndex(), argv);
            if (retVal)
                return retVal;
        }
    }
    return retVal;
}

QT_END_NAMESPACE

#include "moc_qqmlfunctionfilter_p.cpp"
