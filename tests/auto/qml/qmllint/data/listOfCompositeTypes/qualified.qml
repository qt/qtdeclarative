import QtQml
import ListOfCompositeTypes as NS

QtObject {
    id: root
    property int n: 0
    signal go(int k)
    property list<NS.Thing> things: [
        NS.Thing {
            enabled: root.n === 1
            onTriggered: root.go(root.n)
        },
        NS.Thing {
            enabled: root.n === 2
        }
    ]
}
