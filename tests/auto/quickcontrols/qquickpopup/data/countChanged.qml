// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQuick.Controls

Window {
    width: 640
    height: 480
    property bool isModel1: true
    property alias comboBox: comboBox
    property int count: comboBox.count
    ListModel {
        id: model1
        ListElement {
            display: "one"
        }
    }
    ListModel {
        id: model2
        ListElement {
            display: "one"
        }
        ListElement {
            display: "two"
        }
    }
    ComboBox {
        id: comboBox
        model: isModel1 ? model1 : model2
    }
}
