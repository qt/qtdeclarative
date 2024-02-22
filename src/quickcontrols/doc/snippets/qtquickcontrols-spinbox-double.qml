// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//! [1]
SpinBox {
    id: spinBox
    from: 0
    value: decimalToInt(1.1)
    to: decimalToInt(100)
    stepSize: decimalFactor
    editable: true
    anchors.centerIn: parent

    property int decimals: 2
    property real realValue: value / decimalFactor
    readonly property int decimalFactor: Math.pow(10, decimals)

    function decimalToInt(decimal) {
        return decimal * decimalFactor
    }

    validator: DoubleValidator {
        bottom: Math.min(spinBox.from, spinBox.to)
        top:  Math.max(spinBox.from, spinBox.to)
        decimals: spinBox.decimals
        notation: DoubleValidator.StandardNotation
    }

    textFromValue: function(value, locale) {
        return Number(value / decimalFactor).toLocaleString(locale, 'f', spinBox.decimals)
    }

    valueFromText: function(text, locale) {
        return Math.round(Number.fromLocaleString(locale, text) * decimalFactor)
    }
}
//! [1]
