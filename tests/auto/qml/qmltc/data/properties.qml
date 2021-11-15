import QtQml
import QtQuick // for matrix4x4, vectorNd, rect, etc.
QtObject {
    property bool boolP: true
    property double doubleP: 0.5
    property int intP: 42
    property list<QtObject> listQtObjP // always list of QML objects
    property real realP: 2.32
    property string stringP: "hello, world"
    property url urlP: "https://www.qt.io/"
    property var varP: 42.42

    property color colorP: "blue"
    property date dateP
    property font fontP
    property matrix4x4 matrix4x4P
    property point pointP
    property quaternion quatP
    property rect rectP
    property size sizeP
    property vector2d vec2dP
    property vector3d vec3dP
    property vector4d vec4dP

    default property QtObject defaultObjP
    readonly property string readonlyStringP: "foobar"
    required property real requiredRealP

    // extra:
    property Timer timerP
    property list<Component> listNumP

    // special:
    property QtObject nullObjP: null
    property var nullVarP: null
}
