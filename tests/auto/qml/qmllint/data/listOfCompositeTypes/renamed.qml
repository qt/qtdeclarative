import QtQml
import ListOfCompositeTypes

QtObject {
    id: root
    property int n: 0
    signal go(int k)
    property list<Renamed> things: [
        Renamed {
            enabled: root.n === 1
            onTriggered: root.go(root.n)
        },
        Renamed {
            enabled: root.n === 2
        }
    ]
}
