// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQml.XmlListModel

Item {
    id: xmllistmodelelementtest
    anchors.fill: parent
    property string testtext: ""

    XmlListModel { id: xmllistmodelelement
        source: "cookbook.xml"
        query: "/cookbook/recipe"
        XmlListModelRole { name: "title"; elementName: "title" }
        XmlListModelRole { name: "method"; elementName: "method" }
        XmlListModelRole { name: "time"; elementName: "time"; attributeName: "quantity" }
        XmlListModelRole { name: "ingredients"; elementName: "ingredients" }
    }

    ListView {
        id: recipeview
        model: xmllistmodelelement; height: 300; width: 300; clip: true
        anchors.centerIn: parent
        delegate: Component {
            Rectangle { id: delbox; height: 50; width: 296; color: "orange"; border.color: "black"; state: "closed"; clip: true; radius: 5
                anchors.horizontalCenter: parent.horizontalCenter
                Text {
                    id: recipetitle
                    text: model.title; font.pointSize: 12; font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter; y: 20;
                }

                Text {
                    id: recipetime
                    width: parent.width; height: 20; text: "<b>Time: </b>" +model.time + " minutes"; visible: opacity != 0
                    anchors.horizontalCenter: parent.horizontalCenter; anchors.top: recipetitle.bottom
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }

                Item {
                    id: ingredientlist
                    visible: opacity != 0
                    width: parent.width
                    height: header.implicitHeight + content.implicitHeight
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: recipetime.bottom
                        topMargin: 10
                    }
                    Text {
                        id: header
                        text: "<b>Ingredients:</b>"
                    }
                    Text {
                        anchors.top: header.bottom;
                        id: content
                        wrapMode: Text.WordWrap
                        textFormat: Text.MarkdownText
                        width: ingredientlist.width
                        // a bit of regexp to remove unneeded whitespaces
                        text: model.ingredients.replace(/\ +/g,' ')
                    }
                }

                Item {
                    id: recipemethod
                    width: parent.width;
                    height: methodHeader.implicitHeight + methodContent.implicitHeight
                    visible: opacity != 0;
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: ingredientlist.bottom
                    }
                    Text {
                        id: methodHeader
                        text: "<b>Cooking method:</b>"
                    }
                    Text {
                        id: methodContent
                        anchors.top: methodHeader.bottom;
                        wrapMode: Text.WordWrap
                        textFormat: Text.MarkdownText
                        width: recipemethod.width
                        // a bit of regexp to remove unneeded whitespaces
                        text: model.method.replace(/\ +/g,' ')
                    }
                }

                MouseArea { anchors.fill: parent; onClicked: delbox.state = delbox.state == "open" ? "closed" : "open" }
                Behavior on height { NumberAnimation { duration: 250 } }
                states: [
                    State { name: "closed"
                        PropertyChanges { target: delbox; height: 50 }
                        PropertyChanges { target: recipemethod; opacity: 0 }
                        PropertyChanges { target: recipetime; opacity: 0 }
                        PropertyChanges { target: ingredientlist; opacity: 0 }
                    },
                    State { name: "open"
                        PropertyChanges { target: delbox; height: recipemethod.height+recipetime.height+ingredientlist.height+50 }
                        PropertyChanges { target: recipemethod; opacity: 1 }
                        PropertyChanges { target: recipetime; opacity: 1 }
                        PropertyChanges { target: ingredientlist; opacity: 1 }
                        StateChangeScript { script: { recipeview.positionViewAtIndex(model.index, ListView.Beginning); } }
                    }
                ]
            }
        }
    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
        showadvance: false
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: xmllistmodelelementtest
                testtext: "This is a ListView populated by an XmlListModel. Clicking on an item will show the recipe details."
            }
        }
    ]

}
