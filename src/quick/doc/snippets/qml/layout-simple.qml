// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.9
import QtQuick.Layouts 1.2
import QtQuick.Window 2.2

//! [1]
Window {
    RowLayout {
        anchors.fill: parent
        //! [spacing]
        spacing: 6
        //! [spacing]
        Rectangle {
            color: 'azure'
            Layout.preferredWidth: 100
            Layout.preferredHeight: 150
        }
        Rectangle {
            color: "plum"
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
//! [1]
