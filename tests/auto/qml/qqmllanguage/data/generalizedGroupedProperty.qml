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
        root.MyQmlObject.value: 10
        MyQmlObject.value: 4
    }

    property QtObject meanChild: ImmediateProperties {
        child.objectName: "bar"
        child.MyQmlObject.value: 11
    }

    property ImmediateProperties deferred: ImmediateProperties {
        id: aaa
        objectName: "holz"
        root.objectName: objectName + " ..."
        root.MyQmlObject.value2: 12
    }

    property ImmediateProperties meanDeferred: ImmediateProperties {
        aaa.objectName: "stein"
    }
}
