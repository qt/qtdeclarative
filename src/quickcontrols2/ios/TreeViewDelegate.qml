// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS

T.TreeViewDelegate {
    id: control

    implicitWidth: leftMargin + __contentIndent + implicitContentWidth + rightPadding + rightMargin
    implicitHeight: Math.max(implicitBackgroundHeight, implicitContentHeight, implicitIndicatorHeight)

    indentation: indicator ? indicator.width : 12
    leftMargin: 16
    rightMargin: 16
    spacing: 14

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
        implicitWidth: arrow.implicitWidth
        implicitHeight: arrow.implicitHeight

        property Image arrow : Image {
            parent: control.indicator
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            rotation:  control.expanded ? 90 : (control.mirrored ? 180 : 0)
            opacity: control.enabled ? 1 : 0.5

            source: control.IOS.url + "arrow-indicator"
            ImageSelector on source {
                states: [
                    {"light": control.IOS.theme === IOS.Light},
                    {"dark": control.IOS.theme === IOS.Dark}
                ]
            }
        }
    }

    background: Rectangle {
        implicitHeight: 44
        color: control.IOS.theme === IOS.Dark ? control.palette.dark : control.palette.base
        NinePatchImage {
            height: parent.height
            width: parent.width
            source: control.IOS.url + (control.highlighted ? "itemdelegate-background-pressed" : "itemdelegate-background")
            NinePatchImageSelector on source {
                states: [
                    {"light": control.IOS.theme === IOS.Light},
                    {"dark": control.IOS.theme === IOS.Dark}
                ]
            }
        }
    }

    contentItem: Label {
        clip: false
        text: control.model.display
        elide: Text.ElideRight
    }
}
