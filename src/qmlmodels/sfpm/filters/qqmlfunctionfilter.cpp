// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlfunctionfilter_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>
#include <QtQml/private/qqmlobjectcreator_p.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlinfo.h>
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
                function filter(age: int) : bool {
                    return (age <= ageLimit)
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
}

void QQmlFunctionFilter::update(const QQmlSortFilterProxyModel *proxyModel)
{
    Q_D(QQmlFunctionFilter);
    d->parameterCache.reset();
    QQmlFilterBase::update(proxyModel);
}

void QQmlFunctionFilter::componentComplete()
{
    Q_D(QQmlFunctionFilter);
    const auto *metaObj = metaObject();
    for (int idx = metaObj->methodCount() - 1; idx >= 0; idx--) {
        // Once we find the method signature, break the loop
        QMetaMethod method = metaObj->method(idx);
        if (method.nameView() == "filter") {
            d->method = method;
            break;
        }
    }

    if (!d->method.isValid())
        return;

    // Check if the parameter types are valid;
    for (int index = 0; index < d->method.parameterCount(); index++) {
        const QMetaType parameterType = d->method.parameterMetaType(index);
        if (!parameterType.isValid()) {
            qmlWarning(this) << "filter method parameter needs to be a QML-registered type";
            d->method = {};
            return;
        }
    }
}

/*!
    \internal
*/
bool QQmlFunctionFilter::filterAcceptsRowInternal(int row, const QModelIndex& sourceParent, const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlFunctionFilter);
    if (!d->method.isValid() ||
        (d->parameterCache.has_value() && d->parameterCache->dataArgs.isEmpty()))
        return true;

    bool retVal = false;

    if (!d->parameterCache.has_value()) {
        QQmlFunctionFilterPrivate::ParameterCache parameterCache;
        const auto &params = d->method.parameterNames();
        for (int index = 0; index < params.size(); index++) {
            const int roleId = proxyModel->itemRoleForName(QString::fromUtf8(params.at(index)));
            if (roleId < 0) {
                qmlWarning(this) << "Parameter specified in the filter method " << params.at(index) << " doesn't exist in the model";
                d->parameterCache = QQmlFunctionFilterPrivate::ParameterCache{};
                return true;
            }
            parameterCache.paramsInfo.append({roleId, QVariant{}, d->method.parameterMetaType(index)});
        }
        d->parameterCache = std::move(parameterCache);
        // Append nullptr for future utilization of this space to return value
        // while invoking js method (´filter´)
        d->parameterCache->dataArgs.append(nullptr);
        for (auto &param : d->parameterCache->paramsInfo)
            d->parameterCache->dataArgs.append(&param.value);
    }

    d->parameterCache->dataArgs[0] = &retVal;

    auto filterData = [d, proxyModel, this](int row, int column, const QModelIndex &sourceParent) {
        int index = 0;
        for (auto &param : d->parameterCache->paramsInfo) {
            QVariant value = proxyModel->sourceModel()->data(proxyModel->sourceModel()->index(row, column, sourceParent),
                                    param.roleId);
            if (!value.isValid())
                return;
            if (value.metaType() == param.expectedType) {
               param.value = std::move(value);
            } else {
                // Convert according to the JS coercion rules
                auto *v4Engine = qmlEngine(this)->handle();
                QV4::Scope scope(v4Engine);
                QV4::ScopedValue jsVal(scope, v4Engine->metaTypeToJS(value.metaType(), value.constData()));
                QVariant convertedValue(param.expectedType);
                if (!QV4::ExecutionEngine::metaTypeFromJS(jsVal, param.expectedType, convertedValue.data())) {
                    qmlWarning(this) << "Failed to convert param " << d->method.parameterNames()[index] << " to " << param.expectedType.name();
                    return;
                }
                param.value = std::move(convertedValue);
            }
            index++;
        }
        QMetaObject::metacall(
                const_cast<QQmlFunctionFilter *>(this), QMetaObject::InvokeMetaMethod,
                d->method.methodIndex(), d->parameterCache->dataArgs.data());
    };

    if (column() > -1) {
        filterData(row, column(), sourceParent);
    } else {
        const int columnCount = proxyModel->sourceModel()->columnCount(sourceParent);
        for (int column = 0; column < columnCount; column++) {
            filterData(row, column, sourceParent);
             if (retVal)
                return retVal;
        }
    }

    return retVal;
}

QT_END_NAMESPACE

#include "moc_qqmlfunctionfilter_p.cpp"
