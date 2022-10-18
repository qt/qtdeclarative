// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickaccessibleattached_p.h"

#if QT_CONFIG(accessibility)

#include <QtQml/qqmlinfo.h>

#include "private/qquickitem_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Accessible
    \instantiates QQuickAccessibleAttached
    \brief Enables accessibility of QML items.

    \inqmlmodule QtQuick
    \ingroup qtquick-visual-utility
    \ingroup accessibility

    This class is part of the \l {Accessibility for Qt Quick Applications}.

    Items the user interacts with or that give information to the user
    need to expose their information to the accessibility framework.
    Then assistive tools can make use of that information to enable
    users to interact with the application in various ways.
    This enables Qt Quick applications to be used with screen-readers for example.

    The most important properties are \l name, \l description and \l role.

    Example implementation of a simple button:
    \qml
    Rectangle {
        id: myButton
        Text {
            id: label
            text: "next"
        }
        Accessible.role: Accessible.Button
        Accessible.name: label.text
        Accessible.description: "shows the next page"
        Accessible.onPressAction: {
            // do a button click
        }
    }
    \endqml
    The \l role is set to \c Button to indicate the type of control.
    \l {Accessible::}{name} is the most important information and bound to the text on the button.
    The name is a short and consise description of the control and should reflect the visual label.
    In this case it is not clear what the button does with the name only, so \l description contains
    an explanation.
    There is also a signal handler \l {Accessible::pressAction}{Accessible.pressAction} which can be invoked by assistive tools to trigger
    the button. This signal handler needs to have the same effect as tapping or clicking the button would have.

    \sa Accessibility
*/

/*!
    \qmlproperty string QtQuick::Accessible::name

    This property sets an accessible name.
    For a button for example, this should have a binding to its text.
    In general this property should be set to a simple and concise
    but human readable name. Do not include the type of control
    you want to represent but just the name.
*/

/*!
    \qmlproperty string QtQuick::Accessible::description

    This property sets an accessible description.
    Similar to the name it describes the item. The description
    can be a little more verbose and tell what the item does,
    for example the functionality of the button it describes.
*/

/*!
    \qmlproperty enumeration QtQuick::Accessible::role

    This flags sets the semantic type of the widget.
    A button for example would have "Button" as type.
    The value must be one of \l QAccessible::Role.

    Some roles have special semantics.
    In order to implement check boxes for example a "checked" property is expected.

    \table
    \header
        \li \b {Role}
        \li \b {Properties and signals}
        \li \b {Explanation}
    \row
        \li All interactive elements
        \li \l focusable and \l focused
        \li All elements that the user can interact with should have focusable set to \c true and
            set \c focus to \c true when they have the focus. This is important even for applications
            that run on touch-only devices since screen readers often implement a virtual focus that
            can be moved from item to item.
    \row
        \li Button, CheckBox, RadioButton
        \li \l {Accessible::pressAction}{Accessible.pressAction}
        \li A button should have a signal handler with the name \c onPressAction.
            This signal may be emitted by an assistive tool such as a screen-reader.
            The implementation needs to behave the same as a mouse click or tap on the button.
    \row
       \li CheckBox, RadioButton
       \li \l checkable, \l checked, \l {Accessible::toggleAction}{Accessible.toggleAction}

       \li The check state of the check box. Updated on Press, Check and Uncheck actions.
    \row
       \li Slider, SpinBox, Dial, ScrollBar
       \li \c value, \c minimumValue, \c maximumValue, \c stepSize
       \li These properties reflect the state and possible values for the elements.
    \row
       \li Slider, SpinBox, Dial, ScrollBar
       \li \l {Accessible::increaseAction}{Accessible.increaseAction}, \l {Accessible::decreaseAction}{Accessible.decreaseAction}
       \li Actions to increase and decrease the value of the element.
    \endtable
*/

/*! \qmlproperty bool QtQuick::Accessible::focusable
    \brief This property holds whether this item is focusable.

    By default, this property is \c false except for items where the role is one of
    \c CheckBox, \c RadioButton, \c Button, \c MenuItem, \c PageTab, \c EditableText, \c SpinBox, \c ComboBox,
    \c Terminal or \c ScrollBar.
    \sa focused
*/
/*! \qmlproperty bool QtQuick::Accessible::focused
    \brief This property holds whether this item currently has the active focus.

    By default, this property is \c false, but it will return \c true for items that
    have \l QQuickItem::hasActiveFocus() returning \c true.
    \sa focusable
*/
/*! \qmlproperty bool QtQuick::Accessible::checkable
    \brief This property holds whether this item is checkable (like a check box or some buttons).

    By default this property is \c false.
    \sa checked
*/
/*! \qmlproperty bool QtQuick::Accessible::checked
    \brief This property holds whether this item is currently checked.

    By default this property is \c false.
    \sa checkable
*/
/*! \qmlproperty bool QtQuick::Accessible::editable
    \brief This property holds whether this item has editable text.

    By default this property is \c false.
*/
/*! \qmlproperty bool QtQuick::Accessible::searchEdit
    \brief This property holds whether this item is input for a search query.
    This property will only affect editable text.

    By default this property is \c false.
*/
/*! \qmlproperty bool QtQuick::Accessible::ignored
    \brief This property holds whether this item should be ignored by the accessibility framework.

    Sometimes an item is part of a group of items that should be treated as one. For example two labels might be
    visually placed next to each other, but separate items. For accessibility purposes they should be treated as one
    and thus they are represented by a third invisible item with the right geometry.

    For example a speed display adds "m/s" as a smaller label:
    \qml
    Row {
        Label {
            id: speedLabel
            text: "Speed: 5"
            Accessible.ignored: true
        }
        Label {
            text: qsTr("m/s")
            Accessible.ignored: true
        }
        Accessible.role: Accessible.StaticText
        Accessible.name: speedLabel.text + " meters per second"
    }
    \endqml

    \since 5.4
    By default this property is \c false.
*/
/*! \qmlproperty bool QtQuick::Accessible::multiLine
    \brief This property holds whether this item has multiple text lines.

    By default this property is \c false.
*/
/*! \qmlproperty bool QtQuick::Accessible::readOnly
    \brief This property indicates that a text field is read only.

    It is relevant when the role is \l QAccessible::EditableText and set to be read-only.
    By default this property is \c false.
*/
/*! \qmlproperty bool QtQuick::Accessible::selected
    \brief This property holds whether this item is selected.

    By default this property is \c false.
    \sa selectable
*/
/*! \qmlproperty bool QtQuick::Accessible::selectable
    \brief This property holds whether this item can be selected.

    By default this property is \c false.
    \sa selected
*/
/*! \qmlproperty bool QtQuick::Accessible::pressed
    \brief This property holds whether this item is pressed (for example a button during a mouse click).

    By default this property is \c false.
*/
/*! \qmlproperty bool QtQuick::Accessible::checkStateMixed
    \brief This property holds whether this item is in the partially checked state.

    By default this property is \c false.
    \sa checked, checkable
*/
/*! \qmlproperty bool QtQuick::Accessible::defaultButton
    \brief This property holds whether this item is the default button of a dialog.

    By default this property is \c false.
*/
/*! \qmlproperty bool QtQuick::Accessible::passwordEdit
    \brief This property holds whether this item is a password text edit.

    By default this property is \c false.
*/
/*! \qmlproperty bool QtQuick::Accessible::selectableText
    \brief This property holds whether this item contains selectable text.

    By default this property is \c false.
*/

/*!
    \qmlsignal QtQuick::Accessible::pressAction()

    This signal is emitted when a press action is received from an assistive tool such as a screen-reader.
*/
/*!
    \qmlsignal QtQuick::Accessible::toggleAction()

    This signal is emitted when a toggle action is received from an assistive tool such as a screen-reader.
*/
/*!
    \qmlsignal QtQuick::Accessible::increaseAction()

    This signal is emitted when a increase action is received from an assistive tool such as a screen-reader.
*/
/*!
    \qmlsignal QtQuick::Accessible::decreaseAction()

    This signal is emitted when a decrease action is received from an assistive tool such as a screen-reader.
*/
/*!
    \qmlsignal QtQuick::Accessible::scrollUpAction()

    This signal is emitted when a scroll up action is received from an assistive tool such as a screen-reader.
*/
/*!
    \qmlsignal QtQuick::Accessible::scrollDownAction()

    This signal is emitted when a scroll down action is received from an assistive tool such as a screen-reader.
*/
/*!
    \qmlsignal QtQuick::Accessible::scrollLeftAction()

    This signal is emitted when a scroll left action is received from an assistive tool such as a screen-reader.
*/
/*!
    \qmlsignal QtQuick::Accessible::scrollRightAction()

    This signal is emitted when a scroll right action is received from an assistive tool such as a screen-reader.
*/
/*!
    \qmlsignal QtQuick::Accessible::previousPageAction()

    This signal is emitted when a previous page action is received from an assistive tool such as a screen-reader.
*/
/*!
    \qmlsignal QtQuick::Accessible::nextPageAction()

    This signal is emitted when a next page action is received from an assistive tool such as a screen-reader.
*/

QMetaMethod QQuickAccessibleAttached::sigPress;
QMetaMethod QQuickAccessibleAttached::sigToggle;
QMetaMethod QQuickAccessibleAttached::sigIncrease;
QMetaMethod QQuickAccessibleAttached::sigDecrease;
QMetaMethod QQuickAccessibleAttached::sigScrollUp;
QMetaMethod QQuickAccessibleAttached::sigScrollDown;
QMetaMethod QQuickAccessibleAttached::sigScrollLeft;
QMetaMethod QQuickAccessibleAttached::sigScrollRight;
QMetaMethod QQuickAccessibleAttached::sigPreviousPage;
QMetaMethod QQuickAccessibleAttached::sigNextPage;

QQuickAccessibleAttached::QQuickAccessibleAttached(QObject *parent)
    : QObject(parent), m_role(QAccessible::NoRole)
{
    Q_ASSERT(parent);
    if (!item()) {
        qmlWarning(parent) << "Accessible must be attached to an Item";
        return;
    }

    // Enable accessibility for items with accessible content. This also
    // enables accessibility for the ancestors of souch items.
    item()->d_func()->setAccessible();
    QAccessibleEvent ev(item(), QAccessible::ObjectCreated);
    QAccessible::updateAccessibility(&ev);

    if (const QMetaObject *pmo = parent->metaObject()) {
        auto connectPropertyChangeSignal = [parent, pmo, this](
                const char *propertyName, const char *signalName, int slotIndex)
        {
            // basically does this:
            // if the parent has the property \a propertyName with the associated \a signalName:
            //     connect(parent, signalName, this, slotIndex)

            // Note that we explicitly want to only connect to standard property/signal naming
            // convention: "value" & "valueChanged"
            // (e.g. avoid a compound property with e.g. a signal notifier named "updated()")
            int idxProperty = pmo->indexOfProperty(propertyName);
            if (idxProperty != -1) {
                const QMetaProperty property = pmo->property(idxProperty);
                const QMetaMethod signal = property.notifySignal();
                if (signal.name() == signalName)
                    QMetaObject::connect(parent, signal.methodIndex(), this, slotIndex);
            }
            return;
        };
        const QMetaObject &smo = staticMetaObject;
        static const int valueChangedIndex = smo.indexOfSlot("valueChanged()");
        connectPropertyChangeSignal("value", "valueChanged", valueChangedIndex);

        static const int cursorPositionChangedIndex = smo.indexOfSlot("cursorPositionChanged()");
        connectPropertyChangeSignal("cursorPosition", "cursorPositionChanged",
                                    cursorPositionChangedIndex);
    }

    if (!sigPress.isValid()) {
        sigPress = QMetaMethod::fromSignal(&QQuickAccessibleAttached::pressAction);
        sigToggle = QMetaMethod::fromSignal(&QQuickAccessibleAttached::toggleAction);
        sigIncrease = QMetaMethod::fromSignal(&QQuickAccessibleAttached::increaseAction);
        sigDecrease = QMetaMethod::fromSignal(&QQuickAccessibleAttached::decreaseAction);
        sigScrollUp = QMetaMethod::fromSignal(&QQuickAccessibleAttached::scrollUpAction);
        sigScrollDown = QMetaMethod::fromSignal(&QQuickAccessibleAttached::scrollDownAction);
        sigScrollLeft = QMetaMethod::fromSignal(&QQuickAccessibleAttached::scrollLeftAction);
        sigScrollRight = QMetaMethod::fromSignal(&QQuickAccessibleAttached::scrollRightAction);
        sigPreviousPage = QMetaMethod::fromSignal(&QQuickAccessibleAttached::previousPageAction);
        sigNextPage= QMetaMethod::fromSignal(&QQuickAccessibleAttached::nextPageAction);
    }
}

QQuickAccessibleAttached::~QQuickAccessibleAttached()
{
}

void QQuickAccessibleAttached::setRole(QAccessible::Role role)
{
    if (role != m_role) {
        m_role = role;
        Q_EMIT roleChanged();
        // There is no way to signify role changes at the moment.
        // QAccessible::updateAccessibility(parent(), 0, QAccessible::);

        switch (role) {
        case QAccessible::CheckBox:
        case QAccessible::RadioButton:
            if (!m_stateExplicitlySet.focusable)
                m_state.focusable = true;
            if (!m_stateExplicitlySet.checkable)
                m_state.checkable = true;
            break;
        case QAccessible::Button:
        case QAccessible::MenuItem:
        case QAccessible::PageTab:
        case QAccessible::SpinBox:
        case QAccessible::ComboBox:
        case QAccessible::Terminal:
        case QAccessible::ScrollBar:
            if (!m_stateExplicitlySet.focusable)
                m_state.focusable = true;
            break;
        case QAccessible::EditableText:
            if (!m_stateExplicitlySet.editable)
                m_state.editable = true;
            if (!m_stateExplicitlySet.focusable)
                m_state.focusable = true;
            break;
        case QAccessible::StaticText:
            if (!m_stateExplicitlySet.readOnly)
                m_state.readOnly = true;
            if (!m_stateExplicitlySet.focusable)
                m_state.focusable = true;
            break;
        default:
            break;
        }
    }
}

bool QQuickAccessibleAttached::wasNameExplicitlySet() const
{
    return m_nameExplicitlySet;
}

// Allows types to attach an accessible name to an item as a convenience,
// so long as the user hasn't done so themselves.
void QQuickAccessibleAttached::setNameImplicitly(const QString &name)
{
    setName(name);
    m_nameExplicitlySet = false;
}

QQuickAccessibleAttached *QQuickAccessibleAttached::qmlAttachedProperties(QObject *obj)
{
    return new QQuickAccessibleAttached(obj);
}

bool QQuickAccessibleAttached::ignored() const
{
    return item() ? !item()->d_func()->isAccessible : false;
}

void QQuickAccessibleAttached::setIgnored(bool ignored)
{
    if (this->ignored() != ignored && item()) {
        item()->d_func()->isAccessible = !ignored;
        emit ignoredChanged();
    }
}

bool QQuickAccessibleAttached::doAction(const QString &actionName)
{
    QMetaMethod *sig = nullptr;
    if (actionName == QAccessibleActionInterface::pressAction())
        sig = &sigPress;
    else if (actionName == QAccessibleActionInterface::toggleAction())
        sig = &sigToggle;
    else if (actionName == QAccessibleActionInterface::increaseAction())
        sig = &sigIncrease;
    else if (actionName == QAccessibleActionInterface::decreaseAction())
        sig = &sigDecrease;
    else if (actionName == QAccessibleActionInterface::scrollUpAction())
        sig = &sigScrollUp;
    else if (actionName == QAccessibleActionInterface::scrollDownAction())
        sig = &sigScrollDown;
    else if (actionName == QAccessibleActionInterface::scrollLeftAction())
        sig = &sigScrollLeft;
    else if (actionName == QAccessibleActionInterface::scrollRightAction())
        sig = &sigScrollRight;
    else if (actionName == QAccessibleActionInterface::previousPageAction())
        sig = &sigPreviousPage;
    else if (actionName == QAccessibleActionInterface::nextPageAction())
        sig = &sigNextPage;
    if (sig && isSignalConnected(*sig))
        return sig->invoke(this);
    return false;
}

void QQuickAccessibleAttached::availableActions(QStringList *actions) const
{
    if (isSignalConnected(sigPress))
        actions->append(QAccessibleActionInterface::pressAction());
    if (isSignalConnected(sigToggle))
        actions->append(QAccessibleActionInterface::toggleAction());
    if (isSignalConnected(sigIncrease))
        actions->append(QAccessibleActionInterface::increaseAction());
    if (isSignalConnected(sigDecrease))
        actions->append(QAccessibleActionInterface::decreaseAction());
    if (isSignalConnected(sigScrollUp))
        actions->append(QAccessibleActionInterface::scrollUpAction());
    if (isSignalConnected(sigScrollDown))
        actions->append(QAccessibleActionInterface::scrollDownAction());
    if (isSignalConnected(sigScrollLeft))
        actions->append(QAccessibleActionInterface::scrollLeftAction());
    if (isSignalConnected(sigScrollRight))
        actions->append(QAccessibleActionInterface::scrollRightAction());
    if (isSignalConnected(sigPreviousPage))
        actions->append(QAccessibleActionInterface::previousPageAction());
    if (isSignalConnected(sigNextPage))
        actions->append(QAccessibleActionInterface::nextPageAction());
}

QString QQuickAccessibleAttached::stripHtml(const QString &html)
{
#ifndef QT_NO_TEXTHTMLPARSER
    QTextDocument doc;
    doc.setHtml(html);
    return doc.toPlainText();
#else
    return html;
#endif
}

QT_END_NAMESPACE

#include "moc_qquickaccessibleattached_p.cpp"

#endif
