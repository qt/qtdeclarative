import QtQml

QtObject {
    id: root
    objectName: "foo"
    property int i: Math.max(99, 112)

    property Binding child: Binding {
        id: child
        objectName: "barrrrr"
        root.objectName: objectName + " ..."
        root.i: 2
        root.Component.objectName: "foo"
        root.onObjectNameChanged: console.log(root.objectName)
    }

    property Binding meanChild: Binding {
        property string extra: root.objectName + " extra"
        child.objectName: "bar"
        child.Component.objectName: "bar"
        root.i: 3
        when: false
    }
}
