// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Controls.impl
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.TableViewDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    required property int row
    required property int column
    required property var model

    highlighted: control.selected

    leftPadding: styleReader.leftPadding
    rightPadding: styleReader.rightPadding
    topPadding: styleReader.topPadding
    bottomPadding: styleReader.bottomPadding

    leftInset: styleReader.background.leftMargin
    rightInset: styleReader.background.rightMargin
    topInset: styleReader.background.topMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing
    font: styleReader.font

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.TableViewDelegate
        enabled: control.enabled
        // Cell delegates are recycled, so giving one real Qt keyboard focus would
        // clash with what "current" means and in practice activeFocus never
        // triggers here anyway. We map focused to current instead.
        focused: control.current
        checked: control.checked
        hovered: control.hovered
        pressed: control.pressed
        highlighted: control.highlighted
        palette: control.palette
    }

    contentItem: Label {
        clip: false
        text: control.model.display ?? ""
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

    TableView.editDelegate: FocusScope {
        width: parent.width
        height: parent.height

        TableView.onCommit: {
            let model = control.tableView.model
            if (!model)
                return
            const index = model.index(control.row, control.column)
            if (!model.setData(index, textField.text, Qt.EditRole))
                console.warn("The model does not allow setting the EditRole data.")
        }

        Component.onCompleted: textField.selectAll()

        TextField {
            id: textField
            anchors.fill: parent
            text: control.model.edit ?? control.model.display ?? ""
            focus: true
        }
    }

    background: BackgroundDelegate {
        quickControl: control
        backgroundStyle: styleReader.background
    }
}
