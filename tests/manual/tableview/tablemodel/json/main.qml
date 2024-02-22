// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import QtQuick.Window 2.12
import Qt.labs.qmlmodels 1.0

import "JsonData.js" as CachedJsonData

ApplicationWindow {
    id: window
    width: 800
    height: 300
    visible: true

    function requestJson() {
        let doc = new XMLHttpRequest()
        doc.onreadystatechange = function() {
            if (doc.readyState === XMLHttpRequest.DONE) {
                var root = JSON.parse(doc.responseText)
                var race = root.MRData.RaceTable.Races[0]
                var raceResults = race.Results
                var drivers = []
                for (let i = 0; i < raceResults.length; ++i) {
                    drivers.push(raceResults[i].Driver)
                }
                tableView.model.rows = drivers
                print(JSON.stringify(drivers))
            }
        }

        doc.open("GET", "http://ergast.com/api/f1/2005/1/results.json")
        doc.send()
    }

    Component.onCompleted: requestJson()
    // Same as the data we get from ergast. Use it while developing
    // to avoid flooding the server with requests.
//    Component.onCompleted: tableView.model.rows = CachedJsonData.drivers

    ColumnLayout {
        anchors.fill: parent

        TableView {
            id: tableView
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AlwaysOn
            }
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
            }

            Layout.minimumHeight: window.height / 2
            Layout.fillWidth: true
            Layout.fillHeight: true

            model: TableModel {
                TableModelColumn { display: "driverId" }
                TableModelColumn { display: "code" }
                TableModelColumn { display: "url" }
                TableModelColumn { display: "givenName" }
                TableModelColumn { display: "familyName" }
                TableModelColumn { display: "dateOfBirth" }
                TableModelColumn { display: "nationality" }
            }

            delegate: TextField {
                objectName: "tableViewTextFieldDelegate"
                text: model.display
                selectByMouse: true
                implicitWidth: 140
                onAccepted: model.display = text
            }
        }
    }
}
