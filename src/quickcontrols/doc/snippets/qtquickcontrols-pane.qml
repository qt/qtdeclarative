// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

//! [1]
Pane {
    ColumnLayout {
        anchors.fill: parent
        CheckBox { text: qsTr("E-mail") }
        CheckBox { text: qsTr("Calendar") }
        CheckBox { text: qsTr("Contacts") }
    }
}
//! [1]
