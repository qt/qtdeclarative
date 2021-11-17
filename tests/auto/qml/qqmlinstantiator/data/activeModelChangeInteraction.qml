import QtQuick 2.15
import QtQml.Models 2.15

Item {
    id: root
    property int instanceCount: 0
    property alias active: instantiator.active

    ListModel {
        id: listmodel

        dynamicRoles: true
    }

    Component.onCompleted: {
        listmodel.insert(listmodel.count, {name: "one"})
        listmodel.insert(listmodel.count, {name: "two"})
        listmodel.insert(listmodel.count, {name: "three"})
    }

    Instantiator {
        id: instantiator

        active: false

        model: listmodel

        delegate: Text {
            width: 100
            height: 20

            text: name

            Component.onCompleted: ++root.instanceCount
        }

    }
}

