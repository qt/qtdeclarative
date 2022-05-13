// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Imagine
import QtQuick.Controls.Imagine.impl
import QtQuick.Dialogs
import QtQuick.Dialogs.quickimpl
import QtQuick.Layouts

MessageDialogImpl {
    id: control

    // Can't set implicitWidth of the NinePatchImage background, so we do it here.
    implicitWidth: Math.max(320,
                            implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitHeaderWidth,
                            rowLayout.implicitWidth)
    implicitHeight: Math.max(160,
                             implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    topPadding: background ? background.topPadding : 0
    leftPadding: background ? background.leftPadding : 0
    rightPadding: background ? background.rightPadding : 0
    bottomPadding: background ? background.bottomPadding : 0

    topInset: background ? -background.topInset || 0 : 0
    leftInset: background ? -background.leftInset || 0 : 0
    rightInset: background ? -background.rightInset || 0 : 0
    bottomInset: background ? -background.bottomInset || 0 : 0

    MessageDialogImpl.buttonBox: buttonBox
    MessageDialogImpl.detailedTextButton: detailedTextButton

    background: NinePatchImage {
        source: Imagine.url + "dialog-background"
        NinePatchImageSelector on source {
            states: [
                {"modal": control.modal},
                {"dim": control.dim}
            ]
        }
    }

    header: Label {
        text: control.title
        elide: Label.ElideRight
        font.bold: true

        leftPadding: 16
        rightPadding: 16
        topPadding: 12
        height: control.title.length > 0 ? implicitHeight : 0

        background: NinePatchImage {
            width: parent.width
            height: parent.height

            source: Imagine.url + "dialog-title"
            NinePatchImageSelector on source {
                states: [
                    {"modal": control.modal},
                    {"dim": control.dim}
                ]
            }
        }
    }

    contentItem: ColumnLayout {
        Label {
            id: textLabel
            objectName: "textLabel"
            text: control.text

            Layout.margins: 16
        }
        Label {
            id: informativeTextLabel
            objectName: "informativeTextLabel"
            text: control.informativeText

            Layout.margins: 16
        }
    }

    footer: ColumnLayout {
        id: columnLayout

        RowLayout {
            id: rowLayout

            Button {
                id: detailedTextButton
                objectName: "detailedTextButton"
                text: control.showDetailedText ? qsTr("Hide Details...") : qsTr("Show Details...")

                Layout.leftMargin: 16
            }

            DialogButtonBox {
                id: buttonBox
                objectName: "buttonBox"
                spacing: 12
                horizontalPadding: 0
                verticalPadding: 20

                Layout.fillWidth: true
                Layout.leftMargin: detailedTextButton.visible ? 12 : 16
                Layout.rightMargin: 16
            }

            Layout.bottomMargin: 16
        }

        TextArea {
            id: detailedTextArea
            objectName: "detailedText"
            text: control.detailedText
            visible: control.showDetailedText
            wrapMode: TextEdit.WordWrap
            readOnly: true

            padding: 12

            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            background: Rectangle {
                color: Qt.rgba(1,1,1,1)
                radius: 3
                border.color: Qt.darker(control.palette.light)
                border.width: 1
            }
        }
    }

    Overlay.modal: NinePatchImage {
        source: Imagine.url + "dialog-overlay"
        NinePatchImageSelector on source {
            states: [
                {"modal": true}
            ]
        }
    }

    Overlay.modeless: NinePatchImage {
        source: Imagine.url + "dialog-overlay"
        NinePatchImageSelector on source {
            states: [
                {"modal": false}
            ]
        }
    }
}
