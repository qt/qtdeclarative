// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldelegatecomponent_p.h"
#include <QtQmlModels/private/qqmladaptormodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype DelegateChoice
//!    \instantiates QQmlDelegateChoice
    \inqmlmodule Qt.labs.qmlmodels
    \brief Encapsulates a delegate and when to use it.

    The DelegateChoice type wraps a delegate and defines the circumstances
    in which it should be chosen.

    DelegateChoices can be nested inside a DelegateChooser.

    \sa DelegateChooser
*/

/*!
    \qmlproperty variant Qt.labs.qmlmodels::DelegateChoice::roleValue
    This property holds the value used to match the role data for the role provided by \l DelegateChooser::role.
*/
QVariant QQmlDelegateChoice::roleValue() const
{
    return m_value;
}

void QQmlDelegateChoice::setRoleValue(const QVariant &value)
{
    if (m_value == value)
        return;
    m_value = value;
    emit roleValueChanged();
    emit changed();
}

/*!
    \qmlproperty int Qt.labs.qmlmodels::DelegateChoice::row
    This property holds the value used to match the row value of model elements.
    With models that have only the index property (and thus only one column), this property
    should be intended as an index, and set to the desired index value.

    \note Setting both row and index has undefined behavior. The two are equivalent and only
    one should be used.

    \sa index
*/

/*!
    \qmlproperty int Qt.labs.qmlmodels::DelegateChoice::index
    This property holds the value used to match the index value of model elements.
    This is effectively an alias for \l row.

    \sa row
*/
int QQmlDelegateChoice::row() const
{
    return m_row;
}

void QQmlDelegateChoice::setRow(int r)
{
    if (m_row == r)
        return;
    m_row = r;
    emit rowChanged();
    emit indexChanged();
    emit changed();
}

/*!
    \qmlproperty int Qt.labs.qmlmodels::DelegateChoice::column
    This property holds the value used to match the column value of model elements.
*/
int QQmlDelegateChoice::column() const
{
    return m_column;
}

void QQmlDelegateChoice::setColumn(int c)
{
    if (m_column == c)
        return;
    m_column = c;
    emit columnChanged();
    emit changed();
}

QQmlComponent *QQmlDelegateChoice::delegate() const
{
    return m_delegate;
}

/*!
    \qmlproperty Component Qt.labs.qmlmodels::DelegateChoice::delegate
    This property holds the delegate to use if this choice matches the model item.
*/
void QQmlDelegateChoice::setDelegate(QQmlComponent *delegate)
{
    if (m_delegate == delegate)
        return;
    QQmlAbstractDelegateComponent *adc = static_cast<QQmlAbstractDelegateComponent *>(m_delegate);
    if (adc)
        disconnect(adc, &QQmlAbstractDelegateComponent::delegateChanged, this, &QQmlDelegateChoice::delegateChanged);
    m_delegate = delegate;
    adc = static_cast<QQmlAbstractDelegateComponent *>(delegate);
    if (adc)
        connect(adc, &QQmlAbstractDelegateComponent::delegateChanged, this, &QQmlDelegateChoice::delegateChanged);
    emit delegateChanged();
    emit changed();
}

bool QQmlDelegateChoice::match(int row, int column, const QVariant &value) const
{
    if (!m_value.isValid() && m_row < 0 && m_column < 0)
        return true;

    bool roleMatched = true;
    if (m_value.isValid()) {
        roleMatched = (value == m_value);
        if (!roleMatched) {
            bool valueOk = false;
            bool mValueOk = false;
            roleMatched = (value.toInt(&valueOk) == m_value.toInt(&mValueOk) && valueOk && mValueOk);
        }
        if (!roleMatched)
            roleMatched = (value.toString() == m_value.toString());
    }
    const bool rowMatched = (m_row < 0 ) ? true : m_row == row;
    const bool columnMatched = (m_column < 0 ) ? true : m_column == column;
    return roleMatched && rowMatched && columnMatched;
}

/*!
    \qmltype DelegateChooser
//!    \instantiates QQmlDelegateChooser
    \inqmlmodule Qt.labs.qmlmodels
    \brief Allows a view to use different delegates for different types of items in the model.

    The DelegateChooser is a special \l Component type intended for those scenarios where a
   Component is required by a view and used as a delegate. DelegateChooser encapsulates a set of \l
   {DelegateChoice}s. These choices are used to determine the delegate that will be instantiated for
   each item in the model. The selection of the choice is performed based on the value that a model
   item has for \l role, and also based on index.

    DelegateChooser is commonly used when a view needs to display a set of delegates that are
   significantly different from each other. For example, a typical phone settings view might include
   toggle switches, sliders, radio buttons, and other visualizations based on the type of each
   setting. In this case, DelegateChooser could provide an easy way to associate a different type of
   delegate with each setting:

    \qml
    import QtQuick
    import QtQuick.Controls
    import Qt.labs.qmlmodels

    ListView {
        width: 200; height: 400

        ListModel {
            id: listModel
            ListElement { type: "info"; ... }
            ListElement { type: "switch"; ... }
            ListElement { type: "swipe"; ... }
            ListElement { type: "switch"; ... }
        }

        DelegateChooser {
            id: chooser
            role: "type"
            DelegateChoice { roleValue: "info"; ItemDelegate { ... } }
            DelegateChoice { roleValue: "switch"; SwitchDelegate { ... } }
            DelegateChoice { roleValue: "swipe"; SwipeDelegate { ... } }
        }

        model: listModel
        delegate: chooser
    }
    \endqml

    \note This type is intended to transparently work only with TableView and any
   DelegateModel-based view. Views (including user-defined views) that aren't internally based on a
   DelegateModel need to explicitly support this type of component to make it function as described.

    \sa DelegateChoice
*/

/*!
    \qmlproperty string Qt.labs.qmlmodels::DelegateChooser::role
    This property holds the role or the property name used to determine the delegate for a given model item.

    \sa DelegateChoice
*/
void QQmlDelegateChooser::setRole(const QString &role)
{
    if (m_role == role)
        return;
    m_role = role;
    emit roleChanged();
}

/*!
    \qmlproperty list<DelegateChoice> Qt.labs.qmlmodels::DelegateChooser::choices
    \qmldefault

    The list of DelegateChoices for the chooser.

    The list is treated as an ordered list, where the first DelegateChoice to match
    will be used be a view.

    It should not generally be necessary to refer to the \c choices property,
    as it is the default property for DelegateChooser and thus all child items are
    automatically assigned to this property.
*/

QQmlListProperty<QQmlDelegateChoice> QQmlDelegateChooser::choices()
{
    return QQmlListProperty<QQmlDelegateChoice>(this, nullptr,
                                                QQmlDelegateChooser::choices_append,
                                                QQmlDelegateChooser::choices_count,
                                                QQmlDelegateChooser::choices_at,
                                                QQmlDelegateChooser::choices_clear,
                                                QQmlDelegateChooser::choices_replace,
                                                QQmlDelegateChooser::choices_removeLast);
}

void QQmlDelegateChooser::choices_append(QQmlListProperty<QQmlDelegateChoice> *prop, QQmlDelegateChoice *choice)
{
    QQmlDelegateChooser *q = static_cast<QQmlDelegateChooser *>(prop->object);
    q->m_choices.append(choice);
    connect(choice, &QQmlDelegateChoice::changed, q, &QQmlAbstractDelegateComponent::delegateChanged);
    q->delegateChanged();
}

qsizetype QQmlDelegateChooser::choices_count(QQmlListProperty<QQmlDelegateChoice> *prop)
{
    QQmlDelegateChooser *q = static_cast<QQmlDelegateChooser*>(prop->object);
    return q->m_choices.size();
}

QQmlDelegateChoice *QQmlDelegateChooser::choices_at(QQmlListProperty<QQmlDelegateChoice> *prop, qsizetype index)
{
    QQmlDelegateChooser *q = static_cast<QQmlDelegateChooser*>(prop->object);
    return q->m_choices.at(index);
}

void QQmlDelegateChooser::choices_clear(QQmlListProperty<QQmlDelegateChoice> *prop)
{
    QQmlDelegateChooser *q = static_cast<QQmlDelegateChooser *>(prop->object);
    for (QQmlDelegateChoice *choice : q->m_choices)
        disconnect(choice, &QQmlDelegateChoice::changed, q, &QQmlAbstractDelegateComponent::delegateChanged);
    q->m_choices.clear();
    q->delegateChanged();
}

void QQmlDelegateChooser::choices_replace(QQmlListProperty<QQmlDelegateChoice> *prop,
                                          qsizetype index, QQmlDelegateChoice *choice)
{
    QQmlDelegateChooser *q = static_cast<QQmlDelegateChooser *>(prop->object);
    disconnect(q->m_choices[index], &QQmlDelegateChoice::changed,
               q, &QQmlAbstractDelegateComponent::delegateChanged);
    q->m_choices[index] = choice;
    connect(choice, &QQmlDelegateChoice::changed, q,
            &QQmlAbstractDelegateComponent::delegateChanged);
    q->delegateChanged();
}

void QQmlDelegateChooser::choices_removeLast(QQmlListProperty<QQmlDelegateChoice> *prop)
{
    QQmlDelegateChooser *q = static_cast<QQmlDelegateChooser *>(prop->object);
    disconnect(q->m_choices.takeLast(), &QQmlDelegateChoice::changed,
               q, &QQmlAbstractDelegateComponent::delegateChanged);
    q->delegateChanged();
}

QQmlComponent *QQmlDelegateChooser::delegate(QQmlAdaptorModel *adaptorModel, int row, int column) const
{
    QVariant v;
    if (!m_role.isNull())
        v = value(adaptorModel, row, column, m_role);
    if (!v.isValid()) { // check if the row only has modelData, for example if the row is a QVariantMap
        v = value(adaptorModel, row, column, QStringLiteral("modelData"));

        if (v.isValid()) {
            if (v.canConvert(QMetaType(QMetaType::QVariantMap)))
                v = v.toMap().value(m_role);
            else if (v.canConvert(QMetaType(QMetaType::QObjectStar)))
                v = v.value<QObject*>()->property(m_role.toUtf8());
        }
    }

    // loop through choices, finding first one that fits
    for (int i = 0; i < m_choices.size(); ++i) {
        const QQmlDelegateChoice *choice = m_choices.at(i);
        if (choice->match(row, column, v))
            return choice->delegate();
    }

    return nullptr;
}

QT_END_NAMESPACE

#include "moc_qqmldelegatecomponent_p.cpp"
