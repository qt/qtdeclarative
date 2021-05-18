import QtQuick 2.0

Item {
    component IC : Text {
        text: "Hello world!"
        MouseArea {} // Previously this would overwrite the IC
    }

    property IC ic: IC {}
    property string icText: ic.text // Check that we can access a property not contained in the component that would overwrite
}
