// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic

import GaugeHowTo

Control {
    id: control
    spacing: 6
    palette.light: "#ddd"
    palette.accent: "steelblue"

    property alias value: valueRange.value
    property alias minimumValue: valueRange.minimumValue
    property alias maximumValue: valueRange.maximumValue

    /*!
        This property determines the rate at which tickmarks are drawn on the
        gauge. The lower the value, the more often tickmarks are drawn.

        The default value is \c 10.
    */
    property alias tickmarkStepSize: valueRange.stepSize
    property alias position: valueRange.position
    property alias visualPosition: valueRange.visualPosition

    property Component tickmarkLabel: Label {
        text: control.formatValue(value)
        color: control.palette.windowText
        horizontalAlignment: Label.AlignRight

        // These are used by Qt's auto tests to test that this code works.
        // They are not needed in user code unless actually used in the delegate.
        required property int index
        required property real value
    }

    property Component tickmark: Rectangle {
        implicitWidth: control.vertical ? control.tickmarkLength : control.tickmarkWidth
        implicitHeight: control.vertical ? control.tickmarkWidth : control.tickmarkLength
        color: control.palette.light

        // Used by tests.
        required property int index
        required property real value
    }

    property Component minorTickmark: Rectangle {
        implicitWidth: control.vertical ? control.minorTickmarkLength : control.minorTickmarkWidth
        implicitHeight: control.vertical ? control.minorTickmarkWidth : control.minorTickmarkLength
        color: control.palette.light

        // Used by tests.
        required property int index
        required property real value
    }

    /*!
        This property determines the orientation of the gauge.

        The default value is \c Qt.Vertical.
    */
    property int orientation: Qt.Vertical

    readonly property bool vertical: orientation === Qt.Vertical

    /*!
        This property determines the amount of minor tickmarks drawn between
        each regular tickmark.

        The default value is \c 4.
    */
    property int minorTickmarkCount: 4

    readonly property int tickmarkCount: (maximumValue - minimumValue) / tickmarkStepSize + 1
    readonly property real tickmarkSpacing: (tickmarkLabelContainer.height - tickmarkWidth * tickmarkCount) / (tickmarkCount - 1)
    property real tickmarkWidth: 2
    property real tickmarkLength: 16
    readonly property real tickmarkOffset: vertical ? textMetrics.height / 2 : textMetrics.width / 2
    readonly property real minorTickmarkStep: tickmarkStepSize / (minorTickmarkCount + 1);
    property real minorTickmarkWidth: 1
    property real minorTickmarkLength: 8

    property real implicitBarLength: 200
    readonly property real barLength: (vertical ? contentItem.height : contentItem.width) - (tickmarkOffset * 2 - 2)

    ValueRange {
        id: valueRange
        inverted: control.mirrored
    }

    /*!
        This property accepts a function that formats the given \a value for
        display in \l {GaugeStyle::}{tickmarkLabel}.

        For example, to provide a custom format that displays all values with 3
        decimal places:

        \code
        formatValue: function(value) {
            return value.toFixed(3)
        }
        \endcode

        The default function does the following:
        \code
        return Number(value).toLocaleString(control.locale, 'f', 0)
        \endcode
    */
    property var formatValue: function(value) {
        return Number(value).toLocaleString(control.locale, 'f', 0)
    }

    TextMetrics {
        id: textMetrics
        font: control.font
        text: control.formatValue(control.maximumValue)
    }

    contentItem: Item {
        implicitWidth: control.vertical
            ? tickmarkLabelContainer.implicitWidth + control.spacing + tickmarkContainer.implicitWidth
                + control.spacing + valueBarItem.implicitWidth
            : control.implicitBarLength
        implicitHeight: control.vertical
            ? control.implicitBarLength
            : tickmarkLabelContainer.implicitHeight + control.spacing + tickmarkContainer.implicitHeight
                + control.spacing + valueBarItem.implicitHeight

        TickmarkLabelContainer {
            id: tickmarkLabelContainer
            objectName: "tickmarkLabelContainer"
            x: control.vertical ? 0 : (parent.width - width) / 2
            y: control.vertical ? (parent.height - height) / 2 : tickmarkContainer.y + tickmarkContainer.height + control.spacing
            width: control.vertical ? Math.max(implicitWidth, control.tickmarkLength) : control.barLength
            height: control.vertical ? control.barLength : Math.max(implicitHeight, control.tickmarkLength)
            font: control.font
            delegate: control.tickmarkLabel
            count: control.tickmarkCount
            valueRange: valueRange
            orientation: control.orientation
        }

        TickmarkContainer {
            id: tickmarkContainer
            objectName: "tickmarkContainer"
            x: control.vertical ? tickmarkLabelContainer.width + control.spacing : (parent.width - width) / 2
            y: control.vertical ? (parent.height - height) / 2 : valueBarItem.height + control.spacing
            width: control.vertical ? control.tickmarkLength : control.barLength
            height: control.vertical ? control.barLength : control.tickmarkLength
            delegate: control.tickmark
            count: control.tickmarkCount
            valueRange: valueRange
            orientation: control.orientation
        }

        MinorTickmarkContainer {
            objectName: "minorTickmarkContainer"
            x: control.vertical ? tickmarkLabelContainer.width + control.spacing : (parent.width - width) / 2
            y: control.vertical ? (parent.height - height) / 2 : valueBarItem.height + control.spacing
            width: control.vertical ? control.minorTickmarkLength : control.barLength
            height: control.vertical ? control.barLength : control.minorTickmarkLength
            delegate: control.minorTickmark
            count: control.tickmarkCount
            minorCount: control.minorTickmarkCount
            valueRange: valueRange
            orientation: control.orientation
            tickmarkWidth: control.tickmarkWidth
        }

        Rectangle {
            id: valueBarItem
            objectName: "valueBarItem"
            x: control.vertical ? tickmarkContainer.x + tickmarkContainer.width + control.spacing : (parent.width - width) / 2
            y: control.vertical ? (parent.height - height) / 2 : 0
            implicitWidth: control.vertical ? 32 : control.implicitBarLength
            implicitHeight: control.vertical ? control.implicitBarLength : 32
            width: control.vertical ? undefined : control.barLength
            height: control.vertical ? control.barLength : undefined
            color: control.palette.light

            Rectangle {
                y: control.vertical ? Math.round(parent.height - height) : 0
                width: control.vertical ? parent.width : Math.round(control.visualPosition * parent.width)
                height: control.vertical ? Math.round(control.visualPosition * parent.height) : parent.height
                implicitHeight: 140
                color: control.palette.accent
            }
        }
    }
}
