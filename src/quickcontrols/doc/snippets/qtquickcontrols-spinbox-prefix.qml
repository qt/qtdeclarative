// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//! [1]
SpinBox {
    id: spinBox
    from: -100
    value: 11
    to: 100
    editable: true
    anchors.centerIn: parent

    property string prefix: "L="
    property string suffix: "m"

    readonly property regexp numberExtractionRegExp: /\D*?(-?\d*\.?\d*)\D*$/

    validator: RegularExpressionValidator { regularExpression: numberExtractionRegExp }

    textFromValue: function(value, locale) {
        return prefix + Number(value).toLocaleString(locale, 'f', 0) + suffix
    }

    valueFromText: function(text, locale) {
        return Number.fromLocaleString(locale, numberExtractionRegExp.exec(text)[1])
    }
}
//! [1]
