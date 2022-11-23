// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickradiobutton_p.h"
#include "qquickcontrol_p_p.h"
#include "qquickabstractbutton_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RadioButton
    \inherits AbstractButton
//!     \instantiates QQuickRadioButton
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-buttons
    \brief Exclusive radio button that can be toggled on or off.

    \image qtquickcontrols-radiobutton.gif

    RadioButton presents an option button that can be toggled on (checked) or
    off (unchecked). Radio buttons are typically used to select one option
    from a set of options.

    RadioButton inherits its API from \l AbstractButton. For instance,
    you can set \l {AbstractButton::text}{text} and react to
    \l {AbstractButton::clicked}{clicks} using the AbstractButton API.
    The state of the radio button can be set with the
    \l {AbstractButton::}{checked} property.

    Radio buttons are \l {AbstractButton::autoExclusive}{auto-exclusive}
    by default. Only one button can be checked at any time amongst radio
    buttons that belong to the same parent item; checking another button
    automatically unchecks the previously checked one. For radio buttons
    that do not share a common parent, ButtonGroup can be used to manage
    exclusivity.

    \l RadioDelegate is similar to RadioButton, except that it is typically
    used in views.

    \code
    ColumnLayout {
        RadioButton {
            checked: true
            text: qsTr("First")
        }
        RadioButton {
            text: qsTr("Second")
        }
        RadioButton {
            text: qsTr("Third")
        }
    }
    \endcode

    \sa ButtonGroup, {Customizing RadioButton}, {Button Controls}, RadioDelegate
*/

class Q_QUICKTEMPLATES2_PRIVATE_EXPORT QQuickRadioButtonPrivate : public QQuickAbstractButtonPrivate
{
    Q_DECLARE_PUBLIC(QQuickRadioButton)

public:
    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::RadioButton); }
};

QQuickRadioButton::QQuickRadioButton(QQuickItem *parent)
    : QQuickAbstractButton(*(new QQuickRadioButtonPrivate), parent)
{
    setCheckable(true);
    setAutoExclusive(true);
}

QFont QQuickRadioButton::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::RadioButton);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickRadioButton::accessibleRole() const
{
    return QAccessible::RadioButton;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickradiobutton_p.cpp"
