// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.9
import QtQuick.Window 2.2
import "qrc:/quick/shared/" as Examples

Window {
    width: 800
    height: 600
    visible: true
    Examples.LauncherList {
        id: ll
        anchors.fill: parent
        Component.onCompleted: {
            addExample("TextInput", "TextInput properties and signals", Qt.resolvedUrl("textInputPropertiesAndSignals.qml"))
        }
    }
}
