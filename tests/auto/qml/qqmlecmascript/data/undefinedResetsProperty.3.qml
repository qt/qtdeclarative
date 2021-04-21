import Qt.test 1.0

MyQmlObject {
    id: root
    property int counter: 0

    resettableProperty: root.counter == 0 ? 19 : undefined

    function incrementCount() {
        root.counter =  (root.counter + 1) % 3
    }
}
