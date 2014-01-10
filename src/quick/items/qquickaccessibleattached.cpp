/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickaccessibleattached_p.h"

#ifndef QT_NO_ACCESSIBILITY

#include "private/qquickitem_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Accessible
    \instantiates QQuickAccessibleAttached
    \brief Enables accessibility of QML items

    \inqmlmodule QtQuick
    \ingroup qtquick-visual-utility
    \ingroup accessibility

    This class is part of \l {Accessibility for Qt Quick Applications}.

    Items the user interacts with or that give information to the user
    need to expose their information in a semantic way.
    Then assistive tools can make use of that information to enable
    users to interact with the application in various ways.

    This enables Qt Quick applications to be used with screen-readers for example.

    The most important properties to set are \l name and \l role.

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
    for example the functionallity of the button it describes.
*/

/*!
    \qmlproperty enumeration QtQuick::Accessible::role

    This flags sets the semantic type of the widget.
    A button for example would have "Button" as type.
    The value must be one of \l QAccessible::Role .
    Example:
    \qml
    Item {
        id: myButton

        Text {
            id: label
            // ...
        }

        Accessible.name: label.text
        Accessible.role: Accessible.Button

        function accessiblePressAction() {
            //...
        }
    }
    \endqml

    Some roles have special semantics.
    In order to implement check boxes for example a "checked" property is expected.

    \table
    \header
        \li \b {Role}
        \li \b {Expected property}
        \li

    \row
        \li Button
        \li function accessiblePressAction
        \li Called when the button receives a press action. The implementation should visually simulate a button click and perform the button action.
    \row
       \li CheckBox, Radiobutton
       \li checked
       \li The check state of the check box. Updated on Press, Check and Uncheck actions.
    \row
       \li Slider, SpinBox, Dial, ScrollBar
       \li value, minimumValue, maximumValue, stepSize
       \li value will be updated on increase and decrase actions, in accordance with the other properties

    \endtable
*/

/*! \qmlproperty bool focusable
    \brief This property holds whether this item is focusable.

    By default, this property is false except for items where the role is one of
    CheckBox, RadioButton, Button, MenuItem, PageTab, EditableText, SpinBox, ComboBox,
    Terminal or ScrollBar.
*/
/*! \qmlproperty bool focused
    \brief This property holds whether this item currently has the active focus.

    By default, this property is false, but it will return true for items that
    have \l QQuickItem::hasActiveFocus() returning true.
*/
/*! \qmlproperty bool checkable
    \brief This property holds whether this item is checkable (like a check box or some buttons).
*/
/*! \qmlproperty bool checked
    \brief This property holds whether this item is currently checked.
*/
/*! \qmlproperty bool editable
    \brief This property holds whether this item has editable text.
*/
/*! \qmlproperty bool multiLine
    \brief This property holds whether this item has multiple text lines.
*/
/*! \qmlproperty bool readOnly
    \brief This property holds whether this item while being of type \l QAccessible::EditableText
    is set to read-only.
*/
/*! \qmlproperty bool selected
    \brief This property holds whether this item is selected.
*/
/*! \qmlproperty bool selectable
    \brief This property holds whether this item can be selected.
*/
/*! \qmlproperty bool pressed
    \brief This property holds whether this item is pressed (for example a button during a mouse click).
*/
/*! \qmlproperty bool checkStateMixed
    \brief This property holds whether this item is in the partially checked state.
*/
/*! \qmlproperty bool defaultButton
    \brief This property holds whether this item is the default button of a dialog.
*/
/*! \qmlproperty bool passwordEdit
    \brief This property holds whether this item is a password text edit.
*/
/*! \qmlproperty bool selectableText
    \brief This property holds whether this item contains selectable text.
*/

QQuickAccessibleAttached::QQuickAccessibleAttached(QObject *parent)
    : QObject(parent), m_role(QAccessible::NoRole)
{
    Q_ASSERT(parent);
    QQuickItem *item = qobject_cast<QQuickItem*>(parent);
    if (!item)
        return;

    // Enable accessibility for items with accessible content. This also
    // enables accessibility for the ancestors of souch items.
    item->d_func()->setAccessibleFlagAndListener();
    QAccessibleEvent ev(item, QAccessible::ObjectCreated);
    QAccessible::updateAccessibility(&ev);

    if (!parent->property("value").isNull()) {
        connect(parent, SIGNAL(valueChanged()), this, SLOT(valueChanged()));
    }
    if (!parent->property("cursorPosition").isNull()) {
        connect(parent, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
    }
}

QQuickAccessibleAttached::~QQuickAccessibleAttached()
{
}

QQuickAccessibleAttached *QQuickAccessibleAttached::qmlAttachedProperties(QObject *obj)
{
    return new QQuickAccessibleAttached(obj);
}

QT_END_NAMESPACE

#endif
