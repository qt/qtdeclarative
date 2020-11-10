import QtQuick 2
import QtQml.Models 2

Item {
    id: root
    readonly property url url1: "http://qt-project.org"
    property var result1
    property var result2
    ListModel {id: myModel}

    Component.onCompleted: {
        myModel.append({"url": new URL("http://qt.io")})
        myModel.append({"url": url1})
        const entry1 = myModel.get(0)
        root.result1 = entry1.url;
        const entry2 = myModel.get(1)
        root.result2 = entry2.url;
    }
}
