// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktoolbar_p.h"
#include "qquickpane_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ToolBar
    \inherits Pane
//!     \instantiates QQuickToolBar
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-containers
    \brief Container for context-sensitive controls.

    ToolBar is a container of application-wide and context sensitive
    actions and controls, such as navigation buttons and search fields.
    ToolBar is commonly used as a \l {ApplicationWindow::header}{header}
    or a \l {ApplicationWindow::footer}{footer} of an \l ApplicationWindow.

    ToolBar does not provide a layout of its own, but requires you to
    position its contents, for instance by creating a \l RowLayout. If only
    a single item is used within the ToolBar, it will resize to fit the
    implicit size of its contained item. This makes it particularly suitable
    for use together with layouts.

    \image qtquickcontrols-toolbar.png

    \code
    ApplicationWindow {
        visible:true

        header: ToolBar {
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    text: qsTr("‹")
                    onClicked: stack.pop()
                }
                Label {
                    text: "Title"
                    elide: Label.ElideRight
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter
                    Layout.fillWidth: true
                }
                ToolButton {
                    text: qsTr("⋮")
                    onClicked: menu.open()
                }
            }
        }

        StackView {
            id: stack
            anchors.fill: parent
        }
    }
    \endcode

    \sa ApplicationWindow, ToolButton, {Customizing ToolBar}, {Container Controls}
*/

class QQuickToolBarPrivate : public QQuickPanePrivate
{
public:
    QPalette defaultPalette() const override { return QQuickTheme::palette(QQuickTheme::ToolBar); }

    QQuickToolBar::Position position = QQuickToolBar::Header;
};

QQuickToolBar::QQuickToolBar(QQuickItem *parent)
    : QQuickPane(*(new QQuickToolBarPrivate), parent)
{
}

/*!
    \qmlproperty enumeration QtQuick.Controls::ToolBar::position

    This property holds the position of the toolbar.

    \note If the toolbar is assigned as a header or footer of \l ApplicationWindow
    or \l Page, the appropriate position is set automatically.

    Possible values:
    \value ToolBar.Header The toolbar is at the top, as a window or page header.
    \value ToolBar.Footer The toolbar is at the bottom, as a window or page footer.

    The default value is style-specific.

    \sa ApplicationWindow::header, ApplicationWindow::footer, Page::header, Page::footer
*/
QQuickToolBar::Position QQuickToolBar::position() const
{
    Q_D(const QQuickToolBar);
    return d->position;
}

void QQuickToolBar::setPosition(Position position)
{
    Q_D(QQuickToolBar);
    if (d->position == position)
        return;

    d->position = position;
    emit positionChanged();
}

QFont QQuickToolBar::defaultFont() const
{
    return QQuickTheme::font(QQuickTheme::ToolBar);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickToolBar::accessibleRole() const
{
    return QAccessible::ToolBar;
}
#endif

QT_END_NAMESPACE

#include "moc_qquicktoolbar_p.cpp"
