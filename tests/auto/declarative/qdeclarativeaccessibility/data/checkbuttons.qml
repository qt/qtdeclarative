import QtQuick 2.0

Item {
    width: 400
    height: 400

    // button, not checkable
    Rectangle {
        y: 20
        width: 100; height: 20
        Accessible.role : Accessible.Button
    }

    // button, checkable, not checked
    Rectangle {
        y: 40
        width: 100; height: 20
        Accessible.role : Accessible.Button
        property bool checkable: true
        property bool checked: false
    }

    // button, checkable, checked
    Rectangle {
        y: 60
        width: 100; height: 20
        Accessible.role : Accessible.Button
        property bool checkable: true
        property bool checked: true
    }

    // check box, checked
    Rectangle {
        y: 80
        width: 100; height: 20
        Accessible.role : Accessible.CheckBox
        property bool checked: true
    }
    // check box, not checked
    Rectangle {
        y: 100
        width: 100; height: 20
        Accessible.role : Accessible.CheckBox
        property bool checked: false
    }
}

