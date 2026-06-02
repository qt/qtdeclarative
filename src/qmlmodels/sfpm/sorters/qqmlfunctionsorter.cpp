// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQmlModels/private/qqmlfunctionsorter_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>
#include <QtQml/private/qqmlobjectcreator_p.h>
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype FunctionSorter
    \inherits Sorter
    \inqmlmodule QtQml.Models
    \since 6.10
    \preliminary
    \brief Sorts data in a \l SortFilterProxyModel based on the evaluation of
    the designated 'compare' method.

    FunctionSorter allows user to define the designated 'compare' method and it
    will be evaluated to sort the data. The method takes two arguments
    (lhs and rhs) of the specified parameter type and the data can
    be accessed as below for evaluation,

    \qml
    SortFilterProxyModel {
        sourceModel: model
        sorters: [
            FunctionSorter {
                id: functionSorter
                component RoleData: QtObject {
                    property real age
                }
                function compare(lhsData: RoleData, rhsData: RoleData) : int {
                    return (lhsData.age < rhsData.age) ? -1 : ((lhsData === rhsData.age) ? 0 : 1)
                }
            }
        ]
    }
    \endqml

    \note \deprecated [6.12] The \c sort method is deprecated. Rename it to \c compare.

    \note The user needs to explicitly invoke
    \l{SortFilterProxyModel::invalidateSorter} whenever any external qml
    property used within the designated 'compare' method changes. This behaviour
    is subject to change in the future, like implicit invalidation and thus the
    user doesn't need to explicitly invoke
    \l{SortFilterProxyModel::invalidateSorter}.
*/

QQmlFunctionSorter::QQmlFunctionSorter(QObject *parent)
    : QQmlSorterBase (new QQmlFunctionSorterPrivate, parent)
{
}

QQmlFunctionSorter::~QQmlFunctionSorter()
{
    Q_D(QQmlFunctionSorter);
    if (d->m_lhsParameterData.metaType().flags() & QMetaType::PointerToQObject)
        delete d->m_lhsParameterData.value<QObject *>();
    if (d->m_rhsParameterData.metaType().flags() & QMetaType::PointerToQObject)
        delete d->m_rhsParameterData.value<QObject *>();
}

void QQmlFunctionSorter::componentComplete()
{
    Q_D(QQmlFunctionSorter);
    const auto *metaObj = this->metaObject();
    for (int idx = metaObj->methodCount() - 1; idx >= 0; idx--) {
        // Once we find the method signature, break the loop
        QMetaMethod method = metaObj->method(idx);
        if (method.nameView() == "compare") {
            d->m_method = method;
            break;
        } else if (method.nameView() == "sort") {
            qmlWarning(this) << "The 'sort' method is deprecated and will be removed in a future version. Rename it to 'compare'.";
            d->m_method = method;
            break;
        }
    }

    if (!d->m_method.isValid())
        return;

    if (d->m_method.parameterCount() != 2) {
        qmlWarning(this) << d->m_method.name() << " requires two parameters.";
        return;
    }

    QQmlData *data = QQmlData::get(this);
    if (!data || !data->outerContext) {
        qmlWarning(this) << d->m_method.name() << " requires a QML context.";
        return;
    }

    const QMetaType parameterType = d->m_method.parameterMetaType(0);
    if (parameterType != d->m_method.parameterMetaType(1)) {
        qmlWarning(this) << d->m_method.name() << " parameters need to have matching types.";
        return;
    }

    auto cu = QQmlMetaType::obtainCompilationUnit(parameterType);
    const QQmlType parameterQmlType = QQmlMetaType::qmlType(parameterType);

    QQmlRefPointer<QQmlContextData> context = data->outerContext;
    QQmlEngine *engine = context->engine();

    // The code below creates an instance of the inline component, composite,
    // or specific C++ QObject types. The created instance, along with the
    // data, are passed as an arguments to the 'compare' method, which is invoked
    // during the call to QQmlFunctionSorter::compare.
    // To create an instance of required component types (be it inline or
    // composite), an executable compilation unit is required, and this can be
    // obtained by looking up via metatype in the type registry
    // (QQmlMetaType::obtainCompilationUnit). Pass it through the QML engine to
    // make it executable. Further, use the executable compilation unit to run
    // an object creator and produce an instance.
    if (parameterType.flags() & QMetaType::PointerToQObject) {
        QObject *created0 = nullptr;
        QObject *created1 = nullptr;
        if (parameterQmlType.isInlineComponent()) {
            const auto executableCu = engine->handle()->executableCompilationUnit(std::move(cu));
            const QString icName = parameterQmlType.elementName();
            created0 = QQmlObjectCreator(context, executableCu, context, icName).create(
                    executableCu->inlineComponentId(icName), nullptr, nullptr,
                    QQmlObjectCreator::InlineComponent);
            created1 = QQmlObjectCreator(context, executableCu, context, icName).create(
                    executableCu->inlineComponentId(icName), nullptr, nullptr,
                    QQmlObjectCreator::InlineComponent);
        } else if (parameterQmlType.isComposite()) {
            const auto executableCu = engine->handle()->executableCompilationUnit(std::move(cu));
            created0 = QQmlObjectCreator(context, executableCu, context, QString()).create();
            created1 = QQmlObjectCreator(context, executableCu, context, QString()).create();
        } else {
            created0 = parameterQmlType.metaObject()->newInstance();
            created1 = parameterQmlType.metaObject()->newInstance();
        }

        const auto names = d->m_method.parameterNames();
        created0->setObjectName(names[0]);
        created1->setObjectName(names[1]);
        d->m_lhsParameterData = QVariant::fromValue(created0);
        d->m_rhsParameterData = QVariant::fromValue(created1);
    } else {
        d->m_lhsParameterData = QVariant(parameterType);
        d->m_rhsParameterData = QVariant(parameterType);
    }
}

/*!
    \internal
*/
QPartialOrdering QQmlFunctionSorter::compare(
        const QModelIndex& sourceLeft, const QModelIndex& sourceRight,
        const QQmlSortFilterProxyModel *proxyModel) const
{
    Q_D(const QQmlFunctionSorter);
    if (!d->m_method.isValid()
            || !d->m_lhsParameterData.isValid()
            || !d->m_rhsParameterData.isValid()) {
        return QPartialOrdering::Unordered;
    }

    int retVal = 0;
    QSortFilterProxyModelHelper::setProperties(&d->m_lhsParameterData, proxyModel, sourceLeft);
    QSortFilterProxyModelHelper::setProperties(&d->m_rhsParameterData, proxyModel, sourceRight);

    void *argv[] = {&retVal, d->m_lhsParameterData.data(), d->m_rhsParameterData.data()};
    QMetaObject::metacall(
            const_cast<QQmlFunctionSorter *>(this), QMetaObject::InvokeMetaMethod,
            d->m_method.methodIndex(), argv);

    return (retVal == 0)
            ? QPartialOrdering::Equivalent
            : ((retVal < 0) ? QPartialOrdering::Less : QPartialOrdering::Greater);
}

QT_END_NAMESPACE

#include "moc_qqmlfunctionsorter_p.cpp"
