/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

import QtQuick
import QtQuick.Templates as T
import QtQuick.NativeStyle as NativeStyle

T.SpinBox {
    id: control

    property bool __nativeBackground: background instanceof NativeStyle.StyleItem
    property bool nativeIndicators: up.indicator.hasOwnProperty("_qt_default")
                                    && down.indicator.hasOwnProperty("_qt_default")

    font.pixelSize: background.styleFont(control).pixelSize

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            90 /* minimum */ )
    implicitHeight: Math.max(implicitBackgroundHeight, up.implicitIndicatorHeight + down.implicitIndicatorHeight)
                    + topInset + bottomInset

    spacing: 2

    // Push the background right to make room for the indicators
    rightInset: nativeIndicators ? up.implicitIndicatorWidth + spacing : 0

    leftPadding: __nativeBackground ? background.contentPadding.left: 0
    topPadding: __nativeBackground ? background.contentPadding.top: 0
    rightPadding: (__nativeBackground ? background.contentPadding.right : 0) + rightInset
    bottomPadding: __nativeBackground ? background.contentPadding.bottom: 0

    readonly property Item __focusFrameTarget: contentItem

    validator: IntValidator {
        locale: control.locale.name
        bottom: Math.min(control.from, control.to)
        top: Math.max(control.from, control.to)
    }

    contentItem: TextInput {
        text: control.displayText
        font: control.font
        color: control.palette.text
        selectionColor: control.palette.highlight
        selectedTextColor: control.palette.highlightedText
        horizontalAlignment: Qt.AlignLeft
        verticalAlignment: Qt.AlignVCenter

        topPadding: 2
        bottomPadding: 2
        leftPadding: 10
        rightPadding: 10

        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: control.inputMethodHints

        readonly property Item __focusFrameControl: control
    }

    NativeStyle.SpinBox {
        id: upAndDown
        control: control
        subControl: NativeStyle.SpinBox.Up
        visible: nativeIndicators
        x: up.indicator.x
        y: up.indicator.y
        useNinePatchImage: false
    }

    up.indicator: Item {
        x: parent.width - width
        y: (parent.height / 2) - height
        implicitWidth: upAndDown.width
        implicitHeight: upAndDown.height / 2
        property bool _qt_default
    }

    down.indicator: Item {
        x: parent.width - width
        y: up.indicator.y + upAndDown.height / 2
        implicitWidth: upAndDown.width
        implicitHeight: upAndDown.height / 2
        property bool _qt_default
    }

    background: NativeStyle.SpinBox {
        control: control
        subControl: NativeStyle.SpinBox.Frame
        contentWidth: contentItem.implicitWidth
        contentHeight: contentItem.implicitHeight
    }
}
