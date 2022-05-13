// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: slider.implicitWidth
    height: slider.implicitHeight
    visible: true

    property alias slider: slider

    RangeSlider {
        id: slider
        anchors.centerIn: parent
    }
}
