import QtQml
import QtQuick // for matrix4x4, vectorNd, rect, etc.
QtObject {
    property bool boolP
    property double doubleP
    property int intP
    property list<QtObject> listQtObjP // always list of QML objects
    property real realP
    property string stringP
    property url urlP
    property var varP

    property color colorP
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
}
