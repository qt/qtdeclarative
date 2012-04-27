import QtQuick 2.0

Item {
    id: root

    property bool success: false

    Component {
        id: internalComponent

        Item {
            id: internalRoot

            property string foo: ''

            Component.onCompleted: { internalRoot.foo = 'bar' }
            Component.onDestruction: { root.success = (internalRoot.foo == 'bar') }
        }
    }

    Component.onCompleted: {
        internalComponent.createObject()
        gc()
    }
}
