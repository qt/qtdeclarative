import QtQml
import QtQuick // for matrix4x4, vectorNd, rect, etc.
QtObject {
    property bool boolP: true
    property double doubleP: 0.5
    property int intP: 42
    property list<QtObject> listQtObjP // always list of QML objects
    listQtObjP: [
        Text {
            id: listQtObjP_child_0
            text: "child0"
        },
        QtObject {
            property string what: "child1"
        },
        Item {
            Rectangle { id: listQtObjP_child_2_rect }
        }
    ]
    property real realP: 2.32
    property string stringP: "hello, world"
    property url urlP: "https://www.qt.io/"
    property var varP: 42.42

    property color colorP: "blue"
    property date dateP
    property font fontP
    property matrix4x4 matrix4x4P
    property point pointP: ({ x: 100, y: 200 })
    property quaternion quatP: ({ x: 100, y: 200, z: 300, scalar: 400 })
    property rect rectP: ({ x: 100, y: 200, width: 300, height: 400 })
    property size sizeP: ({ width: 100, height: 200 })
    property vector2d vec2dP : ({ x: 100, y: 200 })
    property vector3d vec3dP: ({ x: 100, y: 200, z: 300 })
    property vector4d vec4dP: ({ x: 100, y: 200, z: 300, w: 400 })

    default property QtObject defaultObjP
    readonly property string readonlyStringP: "foobar"
    required property real requiredRealP
    requiredRealP: 3.2

    // extra:
    property Timer timerP: Timer {
        interval: 42
    }
    property list<Component> listCompP

    // special:
    property QtObject nullObjP: null
    property var nullVarP: null

    // Component-wrapped
    property QtObject table: TableView {
        property Component before: Component { Text { text: "beforeDelegate" } }
        delegate: Text { // implicit component
            text: "delegate"
        }
        property Component after: Component { Text { text: "afterDelegate" } }
    }

    property QtObject explicitCompP: Component { // explicit component
        Text {
            id: explicitText
            text: "not a delegate"
        }
    }

    property QtObject sentinelForComponent: QtObject {
        id: sentinel
        property string text: "should be correctly created"
    }
}
