// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.0

Rectangle {
    height: 400
    width: 300
    property int sets: 1

    ListModel {
        id: listmodel
        Component.onCompleted: addNames()
    }

    ListView {
        id: listview
        model: listmodel
        height: 300
        width: 200
        clip: true
        anchors.centerIn: parent

        section.delegate: del1
        section.criteria: ViewSection.FirstCharacter
        section.property: "name"
        delegate: Rectangle {
            height: 50
            width: 200
            color: "gold"
            border.color: "black"
            Text {
                anchors.centerIn: parent
                text: model.name+" ["+model.id+"]"
                color: "black"
                font.bold: true
            }
        }
    }

    function addNames() {
        var names = ["Alpha","Bravo","Charlie","Delta","Echo","Foxtrot",
                     "Golf","Hotel","India","Juliet","Kilo","Lima","Mike",
                     "November","Oscar","Papa","Quebec","Romeo","Sierra","Tango",
                     "Uniform","Victor","Whiskey","XRay","Yankee","Zulu"];
        for (var i=0;i<names.length;++i)
            listmodel.insert(sets*i, {"name":names[i], "id": "id"+i});
        sets++;
    }

    Component {
        id: del1
        Rectangle {
            height: 50
            width: 200
            color: "white"
            border.color: "black"
            border.width: 3
            Text {
                anchors.centerIn: parent
                text: section
            }
        }
    }

    Component {
        id: del2
        Rectangle {
            height: 50
            width: 200
            color: "lightsteelblue"
            border.color: "orange"
            Text {
                anchors.centerIn: parent
                text: section
            }
        }
    }

    Rectangle {
        anchors.fill: listview
        color: "transparent"
        border.color: "green"
        border.width: 3
    }

    Row {
        spacing: 3
        Rectangle {
            height: 40
            width: 70
            color: "blue"
            Text {
                color: "white"
                anchors.centerIn: parent
                text: "Criteria"
            }
            radius: 5
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    listview.section.criteria = listview.section.criteria == ViewSection.FirstCharacter ?
                           ViewSection.FullString : ViewSection.FirstCharacter
                }
            }
        }
        Rectangle {
            height: 40
            width: 70
            color: "blue"
            Text {
                color: "white"
                anchors.centerIn: parent
                text: "Property"
            }
            radius: 5
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    listview.section.property = listview.section.property == "name" ? "id" : "name";
                    console.log(listview.section.property)
                }
            }
        }
        Rectangle {
            height: 40
            width: 75
            color: "blue"
            Text {
                color: "white"
                anchors.centerIn: parent
                text: "Delegate"
            }
            radius: 5
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    console.log("Change delegate")
                    listview.section.delegate = listview.section.delegate == del1 ? del2 : del1
                }
            }
        }
        Rectangle {
            height: 40
            width: 40
            color: "blue"
            Text {
                color: "white"
                anchors.centerIn: parent
                text: "+"
                font.bold: true
            }
            radius: 5
            MouseArea {
                anchors.fill: parent
                onClicked: { addNames(); }
            }
        }
    }
}
