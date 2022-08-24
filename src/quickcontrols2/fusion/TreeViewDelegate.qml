// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.Fusion

T.TreeViewDelegate {
    id: control

    implicitWidth: leftMargin + __contentIndent + implicitContentWidth + rightPadding + rightMargin
    implicitHeight: Math.max(implicitBackgroundHeight, implicitContentHeight, implicitIndicatorHeight)

    indentation: indicator ? indicator.width : 12
    leftMargin: 5
    rightMargin: 5
    spacing: 5

    topPadding: contentItem ? (height - contentItem.implicitHeight) / 2 : 0
    leftPadding: !mirrored ? leftMargin + __contentIndent : width - leftMargin - __contentIndent - implicitContentWidth

    highlighted: control.selected || control.current
               || ((control.treeView.selectionBehavior === TableView.SelectRows
               || control.treeView.selectionBehavior === TableView.SelectionDisabled)
               && control.row === control.treeView.currentRow)

    required property int row
    required property var model
    readonly property real __contentIndent: !isTreeNode ? 0 : (depth * indentation) + (indicator ? indicator.width + spacing : 0)

    indicator: Item {
        readonly property real __indicatorIndent: control.leftMargin + (control.depth * control.indentation)
        x: !control.mirrored ? __indicatorIndent : control.width - __indicatorIndent - width
        y: (control.height - height) / 2
        implicitWidth: Math.max(arrow.implicitWidth, 20)
        implicitHeight: 24 // same as Button.qml

        property ColorImage arrow : ColorImage {
            parent: control.indicator
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            rotation:  control.expanded ? 0 : (control.mirrored ? 90 : -90)
            source: "qrc:/qt-project.org/imports/QtQuick/Controls/Fusion/images/arrow.png"
            color: control.palette.windowText
            defaultColor: "#353637"
        }
    }

    background: Rectangle {
        implicitHeight: 24 // same as Button.qml
        color: control.highlighted
               ? control.palette.highlight
               : (control.treeView.alternatingRows && control.row % 2 !== 0
               ? control.palette.alternateBase : control.palette.base)
    }

    contentItem: Label {
        text: control.model.display
        elide: Text.ElideRight
    }
}
