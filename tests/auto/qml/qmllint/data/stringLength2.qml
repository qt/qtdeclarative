import QtQuick 2.15

Item {
    id: foo

    property string s
    Component.onCompleted: {
        console.log("s.length", foo.s.length);
    }
}
