import QtQuick
import org.qtproject.Test

ListView {
    id: listView
    width: 300
    height: 150
    flickDeceleration: 10000

    model: FetchMoreModel
    delegate: Text {
        height: 50
        text: model.display
    }

    Text {
        anchors.right: parent.right
        text: "count " + listView.count
        color: listView.moving ? "red" : "blue"
    }
}
