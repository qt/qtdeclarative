// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//! [file]
import QtQuick
import QtQuick.Controls.Material

Item {
    id: root
    implicitWidth: 250
    implicitHeight: 250

    enum Mode {
        Hours,
        Minutes
    }

    property int mode: TimePicker.Mode.Hours
    property int hours
    property int minutes
    property bool is24Hour
    property bool interactive: true

    // The mode that the label delegates see, so that we can
    // animate their opacity before their text changes.
    property int __effectiveMode: TimePicker.Mode.Hours
    // For 12 hour pickers, we can use 0 to 60 to represent all values.
    property int __value: 0
    // For 24 hour pickers, we need to store this extra flag.
    property bool __is24HourValueSelected
    // How many values the arm should snap to at a time.
    readonly property int __stepSize: __getStepSize(mode)
    readonly property int __to: 60
    readonly property int __labelAngleStepSize: 360 / 12
    property bool __switchingModes

    // This signal could be used if TimePicker is used as a standalone component.
    // It's emitted when the minute is selected.
    signal accepted()

    // Convenience for setting each property individually, but also
    // ensures that the selector arm is properly rotated when there is
    // a programmatic change in hours or minutes but not mode.
    function openWith(mode, hours, minutes) {
        root.mode = mode
        root.hours = hours
        root.minutes = minutes
        __updateAfterModeOrTimeChange()
    }

    // Until QML gets private properties (QTBUG-11984), use the traditional
    // double-underscore convention.
    function __angleForValue(value: int): real {
        return (value / __to) * 360
    }

    function __getStepSize(mode) {
        return mode === TimePicker.Mode.Hours ? 5 : 1
    }

    function __updateAfterModeOrTimeChange() {
        // We use a function for this rather than a binding, because we could be called before the
        // __stepSize binding is evaluated.
        if (mode === TimePicker.Mode.Hours) {
            // modulo the hours value by __to because we want 60 (12) to be 0.
            __value = hours * __getStepSize(mode) % __to
        } else {
            __value = minutes
        }

        __is24HourValueSelected = mode === TimePicker.Mode.Hours && hours >= 13
    }

    onModeChanged: __updateAfterModeOrTimeChange()

    onIs24HourChanged: {
        // Don't allow 24-hour values when we're not a 24-hour picker.
        if (!is24Hour && hours > 12)
            hours = 12
    }

    // Center dot.
    Rectangle {
        width: 6
        height: 6
        radius: width / 2
        color: Material.primary
        anchors.centerIn: parent
        z: 1
    }

    Rectangle {
        id: contentContainer
        objectName: "contentContainer"
        width: Math.min(parent.width, parent.height)
        height: width
        radius: width / 2
        anchors.centerIn: parent
        color: Material.theme === Material.Light ? "#eeeeee" : "#626262"

        // Animate this so that we don't need an intermediate parent item for the
        // labels to animate the opacity of that instead. That item would be required
        // because we don't want to change the opacity of the contentContainer Rectangle.
        property real labelOpacity: 1

        function updateValueAfterPressPointChange() {
            const y1 = height / 2
            const x1 = width / 2
            const y2 = tapHandler.point.position.y
            const x2 = tapHandler.point.position.x
            const yDistance = y2 - y1
            const xDistance = x2 - x1
            const angle = Math.atan2(yDistance, xDistance)

            let angleInDegrees = (angle * (180 / Math.PI)) + 90.0
            if (angleInDegrees < 0)
                angleInDegrees = 360 + angleInDegrees

            const normalisedAngle = angleInDegrees / 360.0
            const rawValue = normalisedAngle * __to
            // Snap to each step.
            const steppedValue = Math.round(rawValue / __stepSize) * __stepSize
            root.__value = steppedValue
            // Account for the area where the angle wraps around from 360 to 0,
            // otherwise values from 59.5 to 59.999[...] will register as 60 instead of 0.
            if (rawValue > __to - __stepSize / 2)
                root.__value = 0

            const distanceFromCenter = Math.sqrt(Math.pow(xDistance, 2) + Math.pow(yDistance, 2))
            // Only allow selecting 24 hour values when it's in the correct mode.
            root.__is24HourValueSelected = root.is24Hour && root.__effectiveMode === TimePicker.Mode.Hours
                && distanceFromCenter < distanceFromCenterForLabels(true) + selectionIndicator.height * 0.5
        }

        // Returns the distance from our center at which a label should be centered over given is24Hour.
        function distanceFromCenterForLabels(is24Hour) {
            return contentContainer.radius - (is24Hour
                ? selectionIndicator.height * 1.5 : selectionIndicator.height * 0.5)
        }

        states: [
            State {
                name: "hours"
                when: root.mode === TimePicker.Mode.Hours
            },
            State {
                name: "minutes"
                when: root.mode === TimePicker.Mode.Minutes
            }
        ]

        transitions: [
            Transition {
                // When the picker isn't interactive (e.g. when a dialog is opening),
                // we shouldn't animate the opacity of the labels, as it looks wrong,
                // and should only happen when switching between modes while the
                // picker was already visible.
                enabled: root.interactive

                SequentialAnimation {
                    NumberAnimation {
                        target: contentContainer
                        property: "labelOpacity"
                        from: 1
                        to: 0
                        duration: 100
                    }

                    ScriptAction {
                        script: root.__effectiveMode = root.mode
                    }

                    NumberAnimation {
                        target: contentContainer
                        property: "labelOpacity"
                        from: 0
                        to: 1
                        duration: 100
                    }
                }
            },
            Transition {
                enabled: !root.interactive

                // Since the transition above doesn't run when we're not interactive,
                // we need to do the immediate property change here.
                // See QTBUG-13268 for why we use a ScriptAction and not PropertyAction.
                ScriptAction {
                    script: root.__effectiveMode = root.mode
                }
            }

        ]

        TapHandler {
            id: tapHandler
            gesturePolicy: TapHandler.ReleaseWithinBounds
            // Don't allow input while switching modes, or a click on an hour could go through to a minute.
            enabled: root.interactive && root.__effectiveMode === root.mode

            onPointChanged: {
                if (pressed) {
                    // Don't call this when not pressed, as the position will be invalid.
                    contentContainer.updateValueAfterPressPointChange()

                    // Update the value (like a "live" Slider) while the pointer position changes.
                    if (mode === TimePicker.Mode.Hours) {
                        root.hours = root.__value / root.__stepSize

                        if (root.hours === 0) {
                            // A value of 0 (when it's not a 24-hour picker) is 12.
                            // When it is a 24-hour picker, it's 0.
                            if (!root.__is24HourValueSelected)
                                root.hours = 12
                        } else if (root.__is24HourValueSelected) {
                            root.hours += 12
                        }
                    } else {
                        root.minutes = root.__value
                    }
                } else {
                    // Select the value that was chosen in the press code above.
                    if (mode === TimePicker.Mode.Hours) {
                        mode = TimePicker.Mode.Minutes
                    } else {
                        // Does nothing in our example, but could be used to hide the dialog
                        // if it didn't have an OK button to accept it.
                        root.accepted()
                    }
                }
            }
        }

        // The line connecting the center dot to the selection indicator.
        Rectangle {
            id: selectionArm
            objectName: "selectionArm"
            width: 2
            height: contentContainer.distanceFromCenterForLabels(root.__is24HourValueSelected)
            color: Material.primary
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.verticalCenter
            rotation: root.__angleForValue(root.__value)
            transformOrigin: Item.Bottom
            antialiasing: true

            Rectangle {
                id: selectionIndicator
                objectName: "selectionIndicator"
                width: 40
                height: 40
                radius: width / 2
                color: Material.primary
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.top

                Rectangle {
                    width: 4
                    height: 4
                    radius: width / 2
                    color: Material.color(Material.Indigo, Material.Shade100)
                    anchors.centerIn: parent
                    // Only show the circle within the indicator between minute labels.
                    visible: root.__effectiveMode === TimePicker.Mode.Minutes
                        && root.__value % 5 !== 0
                }
            }
        }

        Repeater {
            id: labelRepeater
            model: root.mode === TimePicker.Mode.Hours && root.is24Hour ? 24 : 12
            delegate: Label {
                id: labelDelegate
                text: displayValue
                font.pixelSize: Qt.application.font.pixelSize * (is24HourValue ? 0.85 : 1)
                rotation: -rotationTransform.angle
                opacity: contentContainer.labelOpacity
                anchors.centerIn: parent

                required property int index
                // From 0 to 60.
                readonly property int value: (index * 5) % root.__to
                property int displayValue: root.__effectiveMode === TimePicker.Mode.Hours
                    ? index === 0
                       ? 12
                       : index === 12
                         ? 0
                         : index
                    : value
                // The picker's current value can equal ours but we still might not be current -
                // it depends on whether it's a 24 hour value or not.
                readonly property bool current: root.__value === value && root.__is24HourValueSelected == is24HourValue
                readonly property bool is24HourValue: index >= 12

                Material.foreground: current
                    // When the selection arm is over us, invert our color so it's legible.
                    ? Material.color(Material.Indigo, Material.Shade100)
                    : root.Material.theme === Material.Light
                        ? is24HourValue ? "#686868" : "#484848"
                        : Material.color(Material.Indigo, is24HourValue ? Material.Shade300 : Material.Shade100)

                transform: [
                    Translate {
                        // We're already centered in our parent, so we can use this function
                        // to determine our position, which doesn't need to be aware of our height -
                        // it just needs to tell us where our center position should be.
                        y: -contentContainer.distanceFromCenterForLabels(labelDelegate.is24HourValue)
                    },
                    Rotation {
                        id: rotationTransform
                        angle: root.__angleForValue(labelDelegate.value)
                        origin.x: labelDelegate.width / 2
                        origin.y: labelDelegate.height / 2
                    }
                ]
            }
        }
    }
}
//! [file]
