// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [1]
SpinBox {
    id: spinBox
    from: 0
    to: items.length - 1
    value: 1 // "Medium"

    property var items: ["Small", "Medium", "Large"]

    validator: RegularExpressionValidator {
        regularExpression: new RegExp("(Small|Medium|Large)", "i")
    }

    textFromValue: function(value) {
        return items[value];
    }

    valueFromText: function(text) {
        for (var i = 0; i < items.length; ++i) {
            if (items[i].toLowerCase().indexOf(text.toLowerCase()) === 0)
                return i
        }
        return spinBox.value
    }
}
//! [1]
