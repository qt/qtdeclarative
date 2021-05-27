import QtQuick 2.0
import QtTest 2.0 // qmllint disable unused-imports
import QtTest 2.0 // qmllint disable

Item {
    @Deprecated {}
    property string deprecated

    property string a: root.a // qmllint disable unqualified
    property string b: root.a // qmllint disable

    // qmllint disable unqualified
    property string c: root.a
    property string d: root.a
    // qmllint enable unqualified

    //qmllint disable
    property string e: root.a
    Component.onCompleted: {
        console.log(deprecated);
    }
    // qmllint enable

}
