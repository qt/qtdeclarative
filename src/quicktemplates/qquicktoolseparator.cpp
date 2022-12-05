// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktoolseparator_p.h"

#include "qquickcontrol_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ToolSeparator
    \inherits Control
//!     \instantiates QQuickToolSeparator
    \inqmlmodule QtQuick.Controls
    \since 5.8
    \ingroup qtquickcontrols-separators
    \brief Separates a group of items in a toolbar from adjacent items.

    ToolSeparator is used to visually distinguish between groups of items in a
    toolbar by separating them with a line. It can be used in horizontal or
    vertical toolbars by setting the \l orientation property to \c Qt.Vertical
    or \c Qt.Horizontal, respectively.

    \image qtquickcontrols-toolseparator.png

    \snippet qtquickcontrols-toolseparator.qml 1

    \sa {Customizing ToolSeparator}, {Separator Controls}
*/

class QQuickToolSeparatorPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickToolSeparator)

public:
    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::ToolBar); }

    Qt::Orientation orientation = Qt::Vertical;
};

QQuickToolSeparator::QQuickToolSeparator(QQuickItem *parent)
    : QQuickControl(*(new QQuickToolSeparatorPrivate), parent)
{
}

/*!
    \qmlproperty enumeration QtQuick.Controls::ToolSeparator::orientation

    This property holds the orientation of the tool separator.

    Possible values:
    \value Qt.Horizontal A horizontal separator is used in a vertical toolbar.
    \value Qt.Vertical A vertical separator is used in a horizontal toolbar. (default)
*/
Qt::Orientation QQuickToolSeparator::orientation() const
{
    Q_D(const QQuickToolSeparator);
    return d->orientation;
}

void QQuickToolSeparator::setOrientation(Qt::Orientation orientation)
{
    Q_D(QQuickToolSeparator);
    if (d->orientation == orientation)
        return;

    d->orientation = orientation;
    emit orientationChanged();
}

/*!
    \readonly
    \qmlproperty bool QtQuick.Controls::ToolSeparator::horizontal

    This property holds whether \l orientation is equal to \c Qt.Horizontal.

    It is useful for \l {Customizing ToolSeparator}{customizing ToolSeparator}.

    \sa orientation, vertical
*/
bool QQuickToolSeparator::isHorizontal() const
{
    Q_D(const QQuickToolSeparator);
    return d->orientation == Qt::Horizontal;
}

/*!
    \readonly
    \qmlproperty bool QtQuick.Controls::ToolSeparator::vertical

    This property holds whether \l orientation is equal to \c Qt.Vertical.

    It is useful for \l {Customizing ToolSeparator}{customizing ToolSeparator}.

    \sa orientation, horizontal
*/
bool QQuickToolSeparator::isVertical() const
{
    Q_D(const QQuickToolSeparator);
    return d->orientation == Qt::Vertical;
}

QFont QQuickToolSeparator::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::ToolBar);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickToolSeparator::accessibleRole() const
{
    return QAccessible::Separator;
}
#endif

QT_END_NAMESPACE

#include "moc_qquicktoolseparator_p.cpp"
