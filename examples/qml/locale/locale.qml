// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    width: 320
    height: 480
    color: "lightgray"
    property list<string> locales: ([
                "en_US",
                "en_GB",
                "fi_FI",
                "de_DE",
                "ar_SA",
                "hi_IN",
                "zh_CN",
                "th_TH",
                "fr_FR",
                "nb_NO",
                "sv_SE"
            ])

    component LocaleDelegate: Text {
        required property var modelData
        required property int index

        property string locale: modelData
        height: 30
        width: view.width
        text: `${Qt.locale(modelData).name} (${Qt.locale(modelData).nativeCountryName}/${Qt.locale(modelData).nativeLanguageName})`
        MouseArea {
            anchors.fill: parent
            onClicked: view.currentIndex = parent.index
        }
    }

    property string locale: view.currentIndex === -1 ? "en_US" : root.locales[view.currentIndex]

    Text {
        id: title
        text: "Select locale:"
    }

    Rectangle {
        id: chooser
        anchors.top: title.bottom
        anchors.topMargin: 5
        width: parent.width-10
        x: 5
        height: parent.height/2 - 10
        color: "#40300030"
        ListView {
            id: view
            clip: true
            focus: true
            anchors.fill: parent
            model: root.locales

            delegate: LocaleDelegate {}
            highlight: Rectangle {
                height: 30
                color: "#60300030"
            }
        }
    }

    Rectangle {
        color: "white"
        anchors.top: chooser.bottom
        anchors.topMargin: 5
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        x: 5; width: parent.width - 10

        Column {
            anchors.fill: parent
            spacing: 5
            Text {
                property var date: new Date()
                text: "Date: " + date.toLocaleDateString(Qt.locale(root.locale))
            }
            Text {
                property var date: new Date()
                text: "Time: " + date.toLocaleTimeString(Qt.locale(root.locale))
            }
            Text {
                property var dow: Qt.locale(root.locale).firstDayOfWeek
                text: "First day of week: " + Qt.locale(root.locale).standaloneDayName(dow)
            }
            Text {
                property var num: 10023823
                text: "Number: " + num.toLocaleString(Qt.locale(root.locale))
            }
            Text {
                property var num: 10023823
                text: "Currency: " + num.toLocaleCurrencyString(Qt.locale(root.locale))
            }
        }
    }
}
