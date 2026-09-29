pragma ComponentBehavior: Bound
import QtQml

QtObject {
    id: root

    property QtObject inner: component.createObject(root)

    property Component component: Component {
        QtObject {
            property int value: 5

            function read(): int {
                const v = value
                console.warn("continued")
                return v
            }

            property QtObject child: QtObject {
                function read(): int {
                    const v = value
                    console.warn("continued")
                    return v
                }
            }
        }
    }
}
