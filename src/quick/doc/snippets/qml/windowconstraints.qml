// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Layouts
import QtQuick.Window

Window {
    //! [binddefaultsize]
    width: layout.implicitWidth
    height: layout.implicitHeight
    //! [binddefaultsize]
    //! [bindconstraints]
    minimumWidth: layout.Layout.minimumWidth
    minimumHeight: layout.Layout.minimumHeight
    maximumWidth: 1000
    maximumHeight: layout.Layout.maximumHeight
    //! [bindconstraints]

    //! [rowlayout]
    //! [anchoring]
    RowLayout {
        id: layout
        anchors.fill: parent
    //! [anchoring]
        spacing: 6
        Rectangle {
            color: 'orange'
            Layout.fillWidth: true
            Layout.minimumWidth: 50
            Layout.preferredWidth: 100
            Layout.maximumWidth: 300
            Layout.minimumHeight: 150
            Text {
                anchors.centerIn: parent
                text: parent.width + 'x' + parent.height
            }
        }
        Rectangle {
            color: 'plum'
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            Layout.preferredWidth: 200
            Layout.preferredHeight: 100
            Text {
                anchors.centerIn: parent
                text: parent.width + 'x' + parent.height
            }
        }
    }
    //! [rowlayout]
}
