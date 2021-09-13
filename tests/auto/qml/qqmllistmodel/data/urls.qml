import QtQuick 2
import QtQml.Models 2

Item {
    id: root
    readonly property url url1: "http://qt-project.org"

    property var result1
    property var result2

    property var alive1
    property var alive2

    ListModel {id: myModel}

    Component.onCompleted: {
        myModel.append({"url": new URL("http://qt.io"), "alive": "indeed"})
        myModel.append({"url": url1, "alive": "and kicking"})
        const entry1 = myModel.get(0)
        root.result1 = entry1.url;
        root.alive1 = entry1.alive;
        const entry2 = myModel.get(1)
        root.result2 = entry2.url;
        root.alive2 = entry2.alive;
    }
}
