// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.TreeViewDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding,
                             implicitIndicatorHeight + topPadding + bottomPadding)

    required property int row
    required property var model

    readonly property real __indicatorSpace: indicator
                                            ? implicitIndicatorWidth + styleReader.indicator.leftMargin + styleReader.indicator.rightMargin
                                            : 0
    readonly property real __contentIndent: !isTreeNode
                                            ? 0
                                            : depth * indentation + (indicator ? __indicatorSpace + spacing : 0)

    highlighted: control.selected || control.current
               || ((control.treeView.selectionBehavior === TableView.SelectRows
               || control.treeView.selectionBehavior === TableView.SelectionDisabled)
               && control.row === control.treeView.currentRow)

    indentation: indicator ? __indicatorSpace : 12

    leftPadding: !mirrored ? styleReader.leftPadding + __contentIndent : styleReader.rightPadding
    rightPadding: !mirrored ? styleReader.rightPadding : styleReader.leftPadding + __contentIndent
    topPadding: treeViewLayout.padding.top
    bottomPadding: treeViewLayout.padding.bottom

    leftInset: styleReader.background.leftMargin
    rightInset: styleReader.background.rightMargin
    topInset: styleReader.background.topMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.TreeViewDelegate
        enabled: control.enabled
        // Cell delegates are recycled, so giving one real Qt keyboard focus would
        // clash with what "current" means and in practice activeFocus never
        // triggers here anyway. We map focused to current instead.
        focused: control.current
        checked: control.expanded
        hovered: control.hovered
        pressed: control.pressed
        highlighted: control.highlighted
        palette: control.palette
    }

    StyleKitLayout {
        id: treeViewLayout
        container: control
        // Layout only positions the indicator; content padding is computed above
        contentMargins {
            left: control.mirrored ? styleReader.rightPadding : styleReader.leftPadding + control.depth * control.indentation
            right: control.mirrored ? styleReader.leftPadding + control.depth * control.indentation : styleReader.rightPadding
            top: styleReader.topPadding
            bottom: styleReader.bottomPadding
        }
        layoutItems: [
            // We don't lay out the contentItem here because it occupies the remaining space
            // as calculated by control internal logic.
            StyleKitLayoutItem {
                id: indicatorItem
                item: control.indicator
                alignment: styleReader.indicator.alignment
                margins.left: styleReader.indicator.leftMargin
                margins.right: styleReader.indicator.rightMargin
                margins.top: styleReader.indicator.topMargin
                margins.bottom: styleReader.indicator.bottomMargin
                fillWidth: styleReader.indicator.fillWidth
                fillHeight: styleReader.indicator.fillHeight
            }
        ]
        spacing: styleReader.spacing
        mirrored: control.mirrored
    }

    // Wrap the indicator in an Item, since its visibility is set in two places:
    // updateIndicatorVisibility() in QQuickTreeViewDelegate hides it for nodes
    // without children, and IndicatorDelegate binds it to the style. With the wrapper,
    // the template sets the outer item and the style sets the inner one, so the
    // indicator is only visible when both are visible.
    indicator: Item {
        x: indicatorItem.x
        y: indicatorItem.y
        width: indicatorItem.width
        height: indicatorItem.height
        implicitWidth: indicatorDelegate.implicitWidth
        implicitHeight: indicatorDelegate.implicitHeight

        IndicatorDelegate {
            id: indicatorDelegate
            quickControl: control
            indicatorStyle: styleReader.indicator
            width: parent.width
            height: parent.height
        }
    }

    contentItem: Label {
        clip: false
        text: control.model.display
        elide: Text.ElideRight
        color: styleReader.text.color
        font: styleReader.font
        visible: !control.editing

        horizontalAlignment: styleReader.text.alignment & Qt.AlignHorizontal_Mask
        verticalAlignment: styleReader.text.alignment & Qt.AlignVertical_Mask
        padding: styleReader.text.padding
        leftPadding: styleReader.text.leftPadding
        rightPadding: styleReader.text.rightPadding
        bottomPadding: styleReader.text.bottomPadding
        topPadding: styleReader.text.topPadding
    }

    // The edit delegate is a separate component, and doesn't need
    // to follow the same strict rules that are applied to a control.
    // qmllint disable attached-property-reuse
    // qmllint disable controls-attached-property-reuse
    // qmllint disable QuickControlsSanity.controls-sanity
    TableView.editDelegate: FocusScope {
        width: parent.width
        height: parent.height

        readonly property int __role: {
            let model = control.treeView.model
            let index = control.treeView.index(row, column)
            let editText = model.data(index, Qt.EditRole)
            return editText !== undefined ? Qt.EditRole : Qt.DisplayRole
        }

        TextField {
            id: textField
            x: control.contentItem.x
            y: (parent.height - height) / 2
            width: control.contentItem.width
            text: control.treeView.model.data(control.treeView.index(row, column), __role)
            focus: true
        }

        TableView.onCommit: {
            let index = TableView.view.index(row, column)
            TableView.view.model.setData(index, textField.text, __role)
        }

        Component.onCompleted: textField.selectAll()
    }
    // qmllint enable attached-property-reuse
    // qmllint enable controls-attached-property-reuse
    // qmllint enable QuickControlsSanity.controls-sanity

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
