import QtQml 2.12
import QtQml.Models 2.12
import QtQuick 2.12

Item {
    height: 480
    width: 320
    Rectangle {
        id: rootRect

        function addItem(desc) {
            myModel.append({"desc": desc});
        }

        Rectangle {
            ListView {
                objectName: "listView"
                width: 100
                height: 100
                delegate: Text {
                    required property string desc
                    text: desc
                }
                model: ListModel { id: myModel }
            }
        }

        Component.onCompleted: {
            addItem("Test creation of a delegate with a property");
        }
    }
}
