// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickframe_p.h"
#include "qquickframe_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Frame
    \inherits Pane
//!     \instantiates QQuickFrame
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-containers
    \brief Visual frame for a logical group of controls.

    Frame is used to layout a logical group of controls together within a
    visual frame. Frame does not provide a layout of its own, but requires
    you to position its contents, for instance by creating a \l RowLayout
    or a \l ColumnLayout.

    Items declared as children of a Frame are automatically parented to the
    Frame's \l {Control::}{contentItem}. Items created dynamically need to be
    explicitly parented to the contentItem.

    If only a single item is used within a Frame, it will resize to fit the
    implicit size of its contained item. This makes it particularly suitable
    for use together with layouts.

    \image qtquickcontrols-frame.png

    \snippet qtquickcontrols-frame.qml 1

    \sa {Customizing Frame}, {Container Controls}
*/

QQuickFrame::QQuickFrame(QQuickItem *parent)
    : QQuickPane(*(new QQuickFramePrivate), parent)
{
}

QQuickFrame::QQuickFrame(QQuickFramePrivate &dd, QQuickItem *parent)
    : QQuickPane(dd, parent)
{
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickFrame::accessibleRole() const
{
    return QAccessible::Border;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickframe_p.cpp"
