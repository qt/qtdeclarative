/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.6
import QtQuick.Controls 2.0

AbstractTabBar {
    id: control

    property list<Item> items
    readonly property int count: items.length
    property alias highlight: listView.highlight
    property alias spacing: listView.spacing

    property Component delegate: TabButton {
        width: (listView.width - Math.max(0, count - 1) * spacing) / count
    }

    contentWidth: listView.contentWidth
    contentHeight: listView.contentHeight

    implicitWidth: Math.max(background ? background.implicitWidth : 0, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0, contentHeight + topPadding + bottomPadding)

    Accessible.role: Accessible.PageTabList

    ExclusiveGroup {
        id: group
    }

    contentItem: ListView {
        id: listView

        x: control.leftPadding
        y: control.topPadding
        width: parent.width - control.leftPadding - control.rightPadding
        height: parent.height - control.topPadding - control.bottomPadding

        spacing: 1
        orientation: ListView.Horizontal
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem

        model: control.items
        currentIndex: control.currentIndex

        delegate: Loader {
            sourceComponent: control.delegate
            visible: modelData.Tab.visible
            Binding { target: item; property: "Exclusive.group"; value: group }
            Binding { target: item; property: "text"; value: modelData.Tab.title }
            Binding { target: item; property: "checked"; value: control.currentIndex === index }
            Connections { target: item; onClicked: control.currentIndex = index }
        }

        property bool completed: false
        Component.onCompleted: completed = true

        highlightMoveDuration: completed ? 250 : 0
        highlightResizeDuration: 0
        highlightFollowsCurrentItem: true
        highlight: Item {
            z: 2
            Rectangle {
                height: 4
                width: parent.width
                y: parent.height - height
                color: control.Theme.accentColor
            }
        }
    }

    background: Rectangle {
        implicitWidth: 26
        implicitHeight: 26
        width: listView.width
        border.color: control.Theme.backgroundColor
        border.width: 8
        color: listView.count > 1 ? control.Theme.frameColor : control.Theme.backgroundColor
        Rectangle {
            y: parent.height - height
            width: parent.width
            height: 1
            color: control.Theme.frameColor
        }
    }
}
