// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
