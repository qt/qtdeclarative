import QtQml

QtObject {
    id: root

    component MyItem: QtObject {
        property int value: 10
        onValueChanged: root.objectName = "value: " + value
    }

    property Instantiator i: Instantiator {
        id: loader
        delegate: MyItem {}
        onObjectChanged: (loader.object as MyItem).value = 20
    }
}
