import QtQml

QtObject {
    id: root

    property QtObject inner: component.createObject(root)

    property Component component: Component {
        QtObject {
            property int value: 5
            function method() { return 7 }

            function readValue() { return value }
            function callMethod() { return method() }

            property QtObject child: QtObject {
                function readValue() { return value }
                function callMethod() { return method() }
            }
        }
    }
}
