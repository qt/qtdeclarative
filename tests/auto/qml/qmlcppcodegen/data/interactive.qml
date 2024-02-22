// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
import QtQuick 2.9
import QtQuick.Shapes 1.0

Rectangle {
    id: root

    Row {
        Slider {
            id: widthSlider
        }

        property ShapePath linePath: ShapePath {
            id: lineShapePath
            strokeWidth: widthSlider.value
        }
    }
}
