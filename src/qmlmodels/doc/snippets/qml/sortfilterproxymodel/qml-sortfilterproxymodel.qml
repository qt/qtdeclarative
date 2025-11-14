// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
import QtQuick
import QtQuick.Controls
import QtQml.Models

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Sort Filter Proxy Model")

    //! [sfpm-usage]
    ListModel {
        id: listModel
        ListElement { name: "Adan"; age: 25; department: "Process"; pid: 209711; country: "Norway" }
        ListElement { name: "Hannah"; age: 48; department: "HR"; pid: 154916; country: "Germany" }
        ListElement { name: "Divina"; age: 63; department: "Marketing"; pid: 158038; country: "Spain" }
        ListElement { name: "Rohith"; age: 35; department: "Process"; pid: 202582; country: "India" }
        ListElement { name: "Latesha"; age: 23; department: "Quality"; pid: 232582; country: "UK" }
    }

    SortFilterProxyModel {
        id: ageFilterModel
        model: listModel
        filters: [
            AgeFilter { }
        ]
        sorters: [
            RoleSorter { roleName: "department" }
        ]
    }

    ListView {
        anchors.fill: parent
        clip: true
        model: ageFilterModel
        delegate: Rectangle {
            implicitWidth: 100
            implicitHeight: 50
            border.width: 1
            Text {
                text: name
                anchors.centerIn: parent
            }
        }
    }
    //! [sfpm-usage]
}

//![0]
