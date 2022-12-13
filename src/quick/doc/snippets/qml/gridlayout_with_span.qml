// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

//! [gridlayout-with-span]
ApplicationWindow {
    id: root
    width: 300
    height: 300
    visible: true

    GridLayout {
       rows: 2
       columns: 3
       Rectangle {
           color: 'cyan'
           implicitWidth: 50
           implicitHeight: 50
       }
       Rectangle {
           color: 'magenta'
           implicitWidth: 50
           implicitHeight: 50
       }
       Rectangle {
           color: 'yellow'
           implicitWidth: 50
           implicitHeight: 50
       }
       Rectangle {
           color: 'black'
           implicitWidth: 50
           implicitHeight: 50
           Layout.columnSpan: 3
           Layout.alignment: Qt.AlignHCenter
       }
    }
}
//! [gridlayout-with-span]

