import QtQuick 2.14

Item {
    id: root
    property Item it
    Component.onCompleted: function() {
        let component = Qt.createComponent("requiredNotSet.qml", Component.PreferSynchronous, root)
        console.assert(component.status == Component.Ready)
        root.it = component.createObject(component)
    }
}
