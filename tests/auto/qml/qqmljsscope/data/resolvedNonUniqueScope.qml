import QtQuick

Item {
    Component.onCompleted: {} // `Component` must be resolved
    Component.onDestruction: {}

    property IC p
    p.onXChanged: {
        console.log("hooray");
    }

    component IC : Text {
        property string x
    }
}
