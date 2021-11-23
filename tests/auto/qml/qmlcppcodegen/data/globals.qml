import QtQml

QtObject {
    id: root

    property QtObject application: Qt.application

    property Timer t: Timer {
        interval: 1
        running: true
        onTriggered: Qt.exit(0)
    }

    property Connections c: Connections {
        target: root.application
        function onAboutToQuit() { console.log("End"); }
    }

    Component.onCompleted: console.log("Start", 2, Qt.application.arguments[1]);

    property url somewhere: Qt.resolvedUrl("/somewhere/else.qml")
    property url plain: "/not/here.qml"
    property string somewhereString: somewhere
}
