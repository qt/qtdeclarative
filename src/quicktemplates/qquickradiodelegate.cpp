// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickradiodelegate_p.h"
#include "qquickabstractbutton_p_p.h"
#include "qquickitemdelegate_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RadioDelegate
    \inherits ItemDelegate
//!     \nativetype QQuickRadioDelegate
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-delegates
    \brief Exclusive item delegate with a radio indicator that can be toggled on or off.

    \image qtquickcontrols-radiodelegate.gif

    RadioDelegate presents an item delegate that can be toggled on (checked) or
    off (unchecked). Radio delegates are typically used to select one option
    from a set of options.

    RadioDelegate inherits its API from \l ItemDelegate, which is inherited
    from AbstractButton. For instance, you can set \l {AbstractButton::text}{text},
    and react to \l {AbstractButton::clicked}{clicks} using the AbstractButton
    API. The state of the radio delegate can be set with the
    \l {AbstractButton::}{checked} property.

    Radio delegates are \l {AbstractButton::autoExclusive}{auto-exclusive}
    by default. Only one delegate can be checked at any time amongst radio
    delegates that belong to the same parent item; checking another delegate
    automatically unchecks the previously checked one. For radio delegates
    that do not share a common parent, ButtonGroup can be used to manage
    exclusivity.

    \l RadioButton is similar to RadioDelegate, except that it is typically
    not used in views, but rather when there are only a few options, and often
    with the requirement that each button is uniquely identifiable.

    \code
    ButtonGroup {
        id: buttonGroup
    }

    ListView {
        model: ["Option 1", "Option 2", "Option 3"]
        delegate: RadioDelegate {
            text: modelData
            checked: index == 0
            ButtonGroup.group: buttonGroup
        }
    }
    \endcode

    \sa {Customizing RadioDelegate}, {Delegate Controls}, RadioButton
*/

class Q_QUICKTEMPLATES2_EXPORT QQuickRadioDelegatePrivate : public QQuickItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QQuickRadioDelegate)

public:
    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::ListView); }
};

QQuickRadioDelegate::QQuickRadioDelegate(QQuickItem *parent)
    : QQuickItemDelegate(*(new QQuickRadioDelegatePrivate), parent)
{
    setCheckable(true);
    setAutoExclusive(true);
}

QFont QQuickRadioDelegate::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::ListView);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickRadioDelegate::accessibleRole() const
{
    return QAccessible::RadioButton;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickradiodelegate_p.cpp"
