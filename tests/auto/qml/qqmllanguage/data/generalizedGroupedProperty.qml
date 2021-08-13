import QtQml
import Test

QtObject {
    id: root
    objectName: "foo"
    signal something()

    property QtObject child: ImmediateProperties {
        id: child
        objectName: "barrrrr"
        root.objectName: objectName + " ..."
        root.onSomething: objectName = "rabrab"
    }

    property QtObject meanChild: ImmediateProperties {
        child.objectName: "bar"
    }

    property ImmediateProperties deferred: ImmediateProperties {
        id: aaa
        objectName: "holz"
        root.objectName: objectName + " ..."
    }

    property ImmediateProperties meanDeferred: ImmediateProperties {
        aaa.objectName: "stein"
    }
}
