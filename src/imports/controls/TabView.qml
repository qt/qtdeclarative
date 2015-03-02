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

import QtQuick 2.4
import QtQuick.Controls 2.0
import QtQml.Models 2.1

AbstractTabView {
    id: control

    default property alias items: listView.items
    readonly property alias currentItem: listView.currentItem
    property alias spacing: listView.spacing
    property alias count: listView.count

    contentWidth: listView.implicitWidth
    contentHeight: listView.contentHeight

    implicitWidth: Math.max(background ? background.implicitWidth : 0, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(background ? background.implicitHeight : 0, contentHeight + topPadding + bottomPadding)

    topPadding: tabBar && tabBar.parent === control ? tabBar.height : 0

    tabBar: TabBar { }

    Accessible.role: Accessible.PageTabList

    Binding { target: tabBar; property: "items"; value: control.items }

    contentItem: ListView {
        id: listView

        property list<Item> items

        Binding { target: control; property: "currentIndex"; value: listView.currentIndex }
        Binding { target: listView; property: "currentIndex"; value: control.currentIndex }

        x: control.leftPadding
        y: control.topPadding
        width: parent.width - control.leftPadding - control.rightPadding
        height: parent.height - control.topPadding - control.bottomPadding

        orientation: Qt.Horizontal
        snapMode: ListView.SnapOneItem
        boundsBehavior: Flickable.StopAtBounds
        highlightRangeMode: ListView.StrictlyEnforceRange
        preferredHighlightBegin: 0
        preferredHighlightEnd: 0
        highlightMoveDuration: 250

        model: items

        delegate: Item {
            id: delegate
            width: listView.width
            height: listView.height
            visible: modelData.Tab.visible
            Binding { target: modelData; property: "parent"; value: delegate }
            Binding { target: modelData; property: "Tab.index"; value: index }
            Binding { target: modelData; property: "Tab.view"; value: control }
        }
    }
}
