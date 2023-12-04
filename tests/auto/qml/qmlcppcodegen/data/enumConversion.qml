pragma Strict
import TestTypes

MyType {
    id: root

    property alias status: root.a

    property int test: myEnumType.type
    property bool test_1: myEnumType.type
    objectName: root.status + "m"
}
