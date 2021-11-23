import QtQuick

Item {
    implicitWidth: 12
    property int a: 5
    property int c: child.b + 2
    property int i: child.i * 2

    Item {
        id: child
        property int b: parent.a + 4
        property int i: parent.implicitWidth - 1
    }

    Item {
        id: sibling
        implicitWidth: 29
    }

    Item {
        id: evil
        property string a: "599"
        property string implicitWidth: "444"
    }
}
