import QtQml

QtObject {
    id: root
    signal doneClicked()
    signal foo()

    onObjectNameChanged: foo()
    Component.onCompleted: root.foo.connect(root.doneClicked)
}
