// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    title: "qquickdialog"
    visible: true

    property alias visualizeDialogButtonBoxContentItem: visualizeDialogButtonBoxContentItemMenuItem.checked
    property alias visualizeDialogButtonBox: visualizeDialogButtonBoxMenuItem.checked

    property int dialogSpacing: 60

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                text: "Settings"
                onClicked: settingsMenu.open()

                Menu {
                    id: settingsMenu
                    width: 400

                    MenuItem {
                        id: visualizeDialogButtonBoxContentItemMenuItem
                        text: "Visualize DialogButtonBox contentItem"
                        checkable: true
                    }

                    MenuItem {
                        id: visualizeDialogButtonBoxMenuItem
                        text: "Visualize DialogButtonBox"
                        checkable: true
                    }
                }
            }
        }
    }


    DialogLabel {
        text: "implicit width"
        dialog: dialogImplicitWidthNoButtons
        width: 100
    }
    CustomDialog {
        id: dialogImplicitWidthNoButtons
        x: dialogSpacing
        y: dialogSpacing
        space: 200
    }

    DialogLabel {
        text: "title, implicit width"
        dialog: dialogImplicitWidthTitleNoButtons
        width: 150
    }
    CustomDialog {
        id: dialogImplicitWidthTitleNoButtons
        y: dialogSpacing
        title: "Test"
        previousDialog: dialogImplicitWidthNoButtons
        space: 200
    }

    DialogLabel {
        text: "title, fixed width"
        dialog: dialogFixedWidthTitleNoButtons
    }
    CustomDialog {
        id: dialogFixedWidthTitleNoButtons
        y: dialogSpacing
        width: 300
        title: "Test"
        previousDialog: dialogImplicitWidthTitleNoButtons
        space: 200
    }


    DialogLabel {
        text: "one standard button, implicit width"
        dialog: dialogImplicitWidthOneButton
    }
    CustomDialog {
        id: dialogImplicitWidthOneButton
        x: dialogSpacing
        y: dialogFixedWidthTitleNoButtons.y + dialogFixedWidthTitleNoButtons.height + dialogSpacing
        standardButtons: Dialog.Ok
    }

    DialogLabel {
        text: "two standard buttons, implicit width"
        dialog: dialogImplicitWidthTwoButtons
    }
    CustomDialog {
        id: dialogImplicitWidthTwoButtons
        standardButtons: Dialog.Ok | Dialog.Cancel
        previousDialog: dialogImplicitWidthOneButton
    }

    DialogLabel {
        text: "three standard buttons, implicit width"
        dialog: dialogImplicitWidthThreeButtons
    }
    CustomDialog {
        id: dialogImplicitWidthThreeButtons
        standardButtons: Dialog.Apply | Dialog.RestoreDefaults | Dialog.Cancel
        previousDialog: dialogImplicitWidthTwoButtons
    }


    DialogLabel {
        text: "text, one standard button, implicit width"
        dialog: dialogTextImplicitWidthOneButton
    }
    CustomDialog {
        id: dialogTextImplicitWidthOneButton
        x: dialogSpacing
        y: dialogImplicitWidthThreeButtons.y + dialogImplicitWidthThreeButtons.height + dialogSpacing
        standardButtons: Dialog.Ok

        Label {
            text: "A Label"
        }
    }

    DialogLabel {
        text: "text, two standard buttons, implicit width"
        dialog: dialogTextImplicitWidthTwoButtons
    }
    CustomDialog {
        id: dialogTextImplicitWidthTwoButtons
        standardButtons: Dialog.Ok | Dialog.Cancel
        previousDialog: dialogTextImplicitWidthOneButton

        Label {
            text: "A Label"
        }
    }

    DialogLabel {
        text: "text, three standard buttons, implicit width"
        dialog: dialogTextImplicitWidthThreeButtons
    }
    CustomDialog {
        id: dialogTextImplicitWidthThreeButtons
        standardButtons: Dialog.Apply | Dialog.RestoreDefaults | Dialog.Cancel
        previousDialog: dialogTextImplicitWidthTwoButtons

        Label {
            text: "A Label"
        }
    }


    DialogLabel {
        text: "one standard button, fixed width (300)"
        dialog: dialogFixedWidthOneButton
    }
    CustomDialog {
        id: dialogFixedWidthOneButton
        x: dialogSpacing
        y: dialogTextImplicitWidthThreeButtons.y + dialogTextImplicitWidthThreeButtons.height + dialogSpacing
        width: 300
        standardButtons: Dialog.Ok
    }

    DialogLabel {
        text: "two standard buttons, fixed width (300)"
        dialog: dialogFixedWidthTwoButtons
    }
    CustomDialog {
        id: dialogFixedWidthTwoButtons
        width: 300
        standardButtons: Dialog.Ok | Dialog.Cancel
        previousDialog: dialogFixedWidthOneButton
    }

    DialogLabel {
        text: "three standard buttons, fixed width (300)"
        dialog: dialogFixedWidthThreeButtons
    }
    CustomDialog {
        id: dialogFixedWidthThreeButtons
        width: 300
        standardButtons: Dialog.Apply | Dialog.RestoreDefaults | Dialog.Cancel
        previousDialog: dialogFixedWidthTwoButtons
    }
}
