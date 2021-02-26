import QtQuick
import Qt.test

MyBindable {
    id: root
    property bool toggle: false
    x: 42
    y: 42
    width: 100
    height: 20
    prop: x+y
    states: [
        State {
            name: "on"
            when: root.toggle
            PropertyChanges {
                target: root
                prop: width/height
            }
        }
    ]
}
