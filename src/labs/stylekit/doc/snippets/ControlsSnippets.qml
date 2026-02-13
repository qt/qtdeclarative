// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style: Style {

        //! [control]
        control {
            padding: 8
            background.radius: 4
        }
        //! [control]

        //! [abstractButton]
        abstractButton.background.radius: 4 // all buttons get a radius of 4
        checkBox.indicator.radius: 2 // except check boxes, which get a radius of 2
        //! [abstractButton]

        /* We can only have one theme in the style, so comment out applicationWindow (so that the file compiles)
        //! [applicationWindow]
        light: Theme {
            applicationWindow.background.color: "#f0f0f0"
        }
        dark: Theme {
            applicationWindow.background.color: "#404040"
        }
        //! [applicationWindow]
        */

        //! [button]
        button {
            background {
                implicitWidth: 120
                implicitHeight: 40
                shadow {
                    opacity: 0.6
                    color: "gray"
                    verticalOffset: 2
                    horizontalOffset: 2
                }
                color: "cornflowerblue"
                gradient: Gradient {
                    // The gradient is drawn on top of the 'background.color', so
                    // we use semi-transparent stops to give the button some shading
                    // while letting the base color show through. This makes it easier
                    // to change the color, but keep the shading, in the hovered state below.
                    GradientStop { position: 0.0; color: Qt.alpha("black", 0.0)}
                    GradientStop { position: 1.0; color: Qt.alpha("black", 0.2)}
                }
            }
            text.color: "white"
            hovered.background.color: "royalblue"
            pressed.background.scale: 0.95
        }
        //! [button]

        //! [checkBox]
        checkBox {
            // Hide the button background; show only the indicator and text
            background.visible: false

            indicator {
                color: "white"
                border.color: "darkslategray"
                foreground {
                    color: "transparent"
                    image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"
                    image.color: palette.accent

                    // Hide the checkmark when the checkbox is not checked
                    visible: false
                }
            }

            // Show the checkmark when the checkbox is checked
            checked.indicator.foreground.visible: true
        }
        //! [checkBox]

        //! [comboBox]
        comboBox {
            text.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            background.topLeftRadius: 4
            background.topRightRadius: 4

            indicator {
                // Show only the dropdown arrow, not the indicator background
                color: "transparent"
                border.width: 0
                foreground {
                    image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/drop-indicator.png"
                    image.color: "black"
                }
            }
        }
        //! [comboBox]

        //! [flatButton]
        flatButton {
            // Hide background normally, show on hover
            background.visible: false
            hovered.background.visible: true
        }
        //! [flatButton]

        //! [frame]
        frame {
            background {
                color: "#fafafa"
                border.color: "#d0d0d0"
                radius: 4
            }
        }
        //! [frame]

        //! [groupBox]
        groupBox {
            // A group box is a container for other controls, so we give it some
            // padding to avoid the child controls to be too close to the border
            padding: 20

            // Move the label to the right, to match the radius of the background
            text.leftPadding: 6
            text.color: "darkslategray"

            background {
                // Move the frame down, below the text label
                topMargin: 30
                radius: 6
                color: "#f8f8f8"
                border.color: "#d0d0d0"
            }
        }
        //! [groupBox]

        //! [itemDelegate]
        itemDelegate {
            text.alignment: Qt.AlignVCenter | Qt.AlignLeft
            background {
                radius: 0
                color: "white"
                border.width: 0
            }
            hovered.background.color: "#f0f0f0"
        }
        //! [itemDelegate]

        /* We can only have one theme in the style, so comment out label (so that the file compiles)
        //! [label]
        light: Theme {
            // Labels typically have no background, so set background.visible to false.
            // Alternatively, hiding the background for labels can be done once for all
            // themes by setting Style.label.background.visible: false.
            label.background.visible: false
            label.text.color: "black"
        }
        dark: Theme {
            label.background.visible: false
            label.text.color: "white"
        }
        //! [label]
        */

        //! [page]
        page {
            background {
                color: "white"
                border.width: 0
            }
        }
        //! [page]

        //! [pane]
        pane {
            background {
                implicitWidth: 200
                implicitHeight: 200
                color: "white"
            }
        }
        //! [pane]

        //! [popup]
        popup {
            background {
                color: "white"
                border.width: 1
                border.color: "darkslategray"
                radius: 4
            }
        }
        //! [popup]

        /*
        We cannot include two progressBars in this file, so we
        prefer to compile the more complex indeterminate one.

        //! [progressBar]
        progressBar {
            // Hide the background; show only the progress bar
            background.visible: false
            indicator {
                implicitWidth: 150
                implicitHeight: 8
                radius: 4
                color: "#e0e0e0"
                foreground {
                    // Set margins to add some space between the progress track and the groove
                    margins: 2
                    color: palette.accent
                    radius: 4
                }
            }
        }
        //! [progressBar]
        */

        //! [progressBar indeterminate]
        progressBar {
            indicator {
                implicitWidth: 150
                implicitHeight: 8
                foreground {
                    delegate: StyledItem {
                        // Draw a pulsating progress track when in indeterminate mode, and a normal track otherwise
                        width: control.indeterminate ? parent.width : parent.width * control.visualPosition
                        height: parent.height
                        opacity: control.indeterminate ? 0.1 : 1.0
                        SequentialAnimation on opacity {
                            running: control.indeterminate
                            loops: Animation.Infinite
                            NumberAnimation { from: 0.1; to: 1.0; duration: 500; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: 1.0; to: 0.1; duration: 500; easing.type: Easing.InOutQuad }
                        }
                    }
                }
            }
        }
        //! [progressBar indeterminate]

        //! [radioButton]
        radioButton {
            // Hide the button background; show only the indicator and text
            background.visible: false

            indicator {
                // Make the indicator circular
                radius: 255
                foreground {
                    // The foreground circle is what shows the checked state, so we make
                    // it a bit smaller than the outer circle by setting margins
                    margins: 4
                    radius: 255
                    // Hide the foreground circle when the radio button is not checked
                    visible: false
                }
            }

            // Show the foreground circle when the radio button is checked
            checked.indicator.foreground.visible: true
        }
        //! [radioButton]

        //! [scrollBar]
        scrollBar {
            padding: 0
            background {
                // For scroll bars, we typically don't want a background, only a
                // draggable indicator and a groove.
                visible: false
                // Note: even if the background is hidden, its implicit size is still used to
                // determine the overall size of the scroll bar. So we set implicitHeight equal to
                // the height of the indicator, to make it appear on the side of, and not on top
                // of, the content area in a ScrollView. For a horizontal scroll bar, on the other
                // hand, the width is automatically set by the view to match its own width.
                implicitHeight: 10
            }
            indicator {
                radius: 255
                // For horizontal scroll bars, we only set the height. The width is set by the
                // view to be the relative size of the view's content that is currently visible.
                implicitHeight: 10
                color: "#c0c0c0"
                foreground.visible: false
            }
            vertical {
                // Override properties for vertical scroll bars if needed.
                // For vertical indicators, we only set width. The height is set by the
                // view to be the relative size of the view's content that is currently visible.
                indicator.implicitWidth: 10
                background.implicitWidth: 10
            }
        }
        //! [scrollBar]

        //! [scrollIndicator]
        scrollIndicator {
             // For scroll indicators, we typically don't want a background, only a
             // sliding indicator
            background.visible: false
            indicator {
                // For horizontal indicators, we only set the height. The width is set by the
                // view to be the relative size of the view's content that is currently visible.
                implicitHeight: 5
                border.width: 0
                color: "cornflowerblue"
                // The indicator is so simple that it doesn't need a foreground
                foreground.visible: false
            }
            vertical {
                // Override properties for vertical scroll indicators if needed
                indicator.color: "violet"
                // For vertical indicators, we only set width. The height is set by the
                // view to be the relative size of the view's content that is currently visible.
                indicator.implicitWidth: 5
            }
        }
        //! [scrollIndicator]

        //! [slider]
        slider {
            background.visible: false

            indicator {
                // The groove of the slider should fill out the entire width of
                // the control, which we achieve by setting implicitWidth to Style.Stretch.
                implicitWidth: Style.Stretch
                implicitHeight: 8
                radius: 255
                color: "#e0e0e0"
                foreground {
                    radius: 255
                    color: palette.accent
                }
            }

            handle {
                implicitWidth: 24
                implicitHeight: 24
                radius: 255
                color: "white"
                border.color: "#c0c0c0"
            }

            vertical {
                // if needed, you can override properties for vertical sliders
            }
        }
        //! [slider]

        //! [spinBox]
        spinBox {
            // Center the text between the up/down buttons
            text.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            indicator {
                color: "transparent"
                border.width: 0
                foreground {
                    image.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/arrow-indicator.png"
                    image.color: "black"
                    alignment: Qt.AlignCenter
                }
                // Place the down and up buttons on the left and right side of the control
                down {
                    alignment: Qt.AlignLeft
                    foreground.rotation: 90
                }
                up {
                    alignment: Qt.AlignRight
                    foreground.rotation: -90
                }
            }
        }
        //! [spinBox]

        //! [scrollView]
        scrollView {
            // Add some space between the scroll bars and the content area
            padding: 2
            // For the sake of the example, style the scroll bars in a ScrollView
            // differently from the default scroll bars by using a StyleVariation.
            variations: StyleVariation {
                scrollBar.indicator.color: "cornflowerblue"
            }
        }
        //! [scrollView]

        //! [switchControl]
        switchControl {
            // For a switch control, we typically don't want a background, only an indicator with a handle
            background.visible: false
            indicator {
                radius: 12
                foreground {
                    // Add some space between the indicator and the foreground track
                    margins: 2
                    radius: 12
                    // The foreground track should only be visible when the switch is checked
                    visible: false
                }
            }
            checked.indicator.foreground.visible: true
            handle {
                // Add some space between the handle and the indicator
                leftMargin: 2
                rightMargin: 2
                implicitWidth: 20
                implicitHeight: 20
                radius: 255
                color: "white"
            }
        }
        //! [switchControl]

        //! [tabBar]
        tabBar {
            padding: 0
            spacing: -1 // let tab buttons overlap slightly
            background {
                color: "white"
                border.width: 0
            }
        }
        //! [tabBar]

        //! [tabButton]
        tabButton {
            background {
                radius: 0
                topLeftRadius: 4
                topRightRadius: 4
                color: "#e8e8e8"
            }
            checked.background.color: "white"
        }
        //! [tabButton]

        //! [textArea]
        textArea {
            // Add some space between the text and the border of the text area
            padding: 8
            text.color: "black"
            background {
                color: "white"
                border.color: "#c0c0c0"
            }
            focused.background.border.color: palette.accent
        }
        //! [textArea]

        //! [textField]
        textField {
            // Add some space between the text and the border of the text field
            padding: 8
            text.alignment: Qt.AlignVCenter
            background {
                implicitWidth: 150
                color: "white"
                border.color: "#c0c0c0"
            }
            focused.background.border.color: palette.accent
        }
        //! [textField]

        //! [textInput]
        textInput {
            // Add some space between the text and the border of the text input
            padding: 8
            text.color: "black"
            background {
                color: "white"
                border.width: 1
                border.color: "#c0c0c0"
            }
        }
        //! [textInput]

        //! [toolBar]
        toolBar {
            spacing: 2
            background {
                radius: 0
                implicitHeight: 40
                border.width: 1
            }
        }
        //! [toolBar]

        //! [toolButton]
        toolButton {
            background.radius: 0
        }
        //! [toolButton]

        //! [toolSeparator]
        toolSeparator {
            background.visible: false
            indicator {
                implicitWidth: 30
                implicitHeight: 1
                border.width: 0
                color: "#c0c0c0"
                foreground.visible: false
            }
            vertical {
                // Override properties for vertical tool separators if needed.
                // Note: Qt Quick Controls automatically swaps the width and height
                // for vertical tool separators, so avoid doing that here.
            }
        }
        //! [toolSeparator]

        //! [theme]
        light: Theme {
            applicationWindow.background.color: "#f0f0f0"
            label.text.color: "black"
        }

        dark: Theme {
            applicationWindow.background.color: "#404040"
            label.text.color: "white"
        }
        //! [theme]
    }

    // The rest of the file is not a part of the docs, it just creates a simple
    // UI to test the doc style from the command line: qml ControlsSnippets.qml

    ScrollView {
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            ToolBar {
                id: tb
                Row {
                    spacing: tb.spacing
                    anchors.fill: parent
                    ToolButton { text: "Tool 1" }
                    ToolButton { text: "Tool 2" }
                    ToolSeparator { anchors.verticalCenter: parent.verticalCenter }
                    ToolButton { text: "Tool 3" }
                }
            }

            ComboBox {
                // The user can choose which theme to use
                model: StyleKit.style.themeNames
                onCurrentTextChanged: StyleKit.style.themeName = currentText
            }

            Button {
                text: "Button"
            }

            Slider {
                width: 200
            }

            Slider {
                height: 200
                orientation: Qt.Vertical
            }

            CheckBox {
                text: "CheckBox"
            }

            RadioButton {
                text: "RadioButton"
                checked: true
            }

            RadioButton {
                text: "RadioButton"
            }

            RadioButton {
                text: "RadioButton"
            }

            Frame {
                width: 100
                height: 100
                clip: true
                Flickable {
                    anchors.fill: parent
                    contentWidth: 200
                    contentHeight: 200
                    ScrollIndicator.vertical: ScrollIndicator {}
                    ScrollIndicator.horizontal: ScrollIndicator {}
                    Text {
                        text: "Test ScrollIndicators"
                        font.pixelSize: 20
                    }
                }
            }

            SpinBox {
                from: 0
                to: 100
                value: 50
            }

            Switch {
                text: "Switch"
            }

            TabBar {
                TabButton { text: "Tab 1" }
                TabButton { text: "Tab 2" }
                TabButton { text: "Tab 3" }
            }

            TextArea {
                width: 200
                height: 100
                text: "TextArea"
            }

            TextField {
                width: 200
                text: "TextField"
            }
        }
    }
}
