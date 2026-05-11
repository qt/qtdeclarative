import QtQuick 2.0

Item {
    width: 400
    height: 400
    Accessible.name: "root"

    // button, not expandable
    Rectangle {
        objectName: "button1"
        y: 20
        width: 100; height: 20
        Accessible.role : Accessible.Button
    }

    // button, expandable, not expanded
    Rectangle {
        objectName: "button2"
        y: 40
        width: 100; height: 20
        Accessible.role : Accessible.Button
        Accessible.expandable: expandable
        Accessible.expanded: expanded
        property bool expandable: true
        property bool expanded: false
    }

    // button, expandable, expanded
    Rectangle {
        objectName: "button3"
        y: 60
        width: 100; height: 20
        Accessible.role : Accessible.Button
        Accessible.expandable: expandable
        Accessible.expanded: expanded
        property bool expandable: true
        property bool expanded: true
    }
}

