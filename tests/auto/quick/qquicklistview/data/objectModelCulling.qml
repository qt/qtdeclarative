import QtQuick 2.12

import QtQml.Models 2.12

Item {

    ObjectModel {
        id: model1
        objectName: "model1"

        Rectangle {
           color: "red"
           objectName: "redRect"
           height: 30; width: 80;
        }
    }

    ObjectModel {
        id: model2
        objectName: "model2"

        Rectangle {
           color: "green"
           height: 30; width: 80;
           x:10
        }
    }

    ListView {
        id: listView
        objectName: "lv"

        anchors.fill: parent 
    }
}
