import QtQuick
import Qt.labs.qmlmodels
import QtQml.Models

Item {
    id: root
    property bool triggered: false
    onTriggeredChanged: {
        rootLM.setProperty(1, "currentRole", "first");
    }
    width: 800
    height: 680

    function verify(): bool {
        rootLV.currentIndex = 1; // needed for itemAtIndex to work
        if (root.triggered)
            return rootLV.itemAtIndex(0).isFirst && rootLV.itemAtIndex(1).isFirst;
        else
            return rootLV.itemAtIndex(0).isFirst && !rootLV.itemAtIndex(1).isFirst;
    }

    ListModel {
        id: rootLM
        ListElement {
            currentRole: "first"
            firstText: "TEXT_FIRST_1"
            secondText: "TEXT_SECOND_1"
        }
        ListElement {
            currentRole: "second"
            firstText: "TEXT_FIRST_2"
            secondText: "TEXT_SECOND_2"
        }
    }

    DelegateModel {
        id: delModel
        model: rootLM
        delegate: DelegateChooser {
            id: delegateChooser
            role: "currentRole"
            DelegateChoice {
                roleValue: "first"
                Rectangle {
                    property bool isFirst: true
                    height: 30
                    width: rootLV.width
                    color: "yellow"
                    Text {
                        anchors.centerIn: parent
                        text: firstText + "    " + currentRole
                    }
                }
            }
            DelegateChoice {
                roleValue: "second"
                Rectangle {
                    property bool isFirst: false
                    height: 30
                    width: rootLV.width
                    color: "red"
                    Text {
                        anchors.centerIn: parent
                        text: secondText + "    " + currentRole
                    }
                }
            }
        }
    }

    TapHandler {
        // for manual testing
        onTapped: root.triggered = true
    }

    Rectangle {
        width: 200
        height: 300
        anchors.centerIn: parent
        border.color: "black"
        border.width: 1
        color: "blue"

        ListView {
            id: rootLV
            objectName: "listview"
            anchors.margins: 30
            anchors.fill: parent
            cacheBuffer: 0
            model: delModel
        }
    }
}
