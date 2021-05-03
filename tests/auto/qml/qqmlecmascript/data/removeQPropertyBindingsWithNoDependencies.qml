import Qt.test 1

ClassWithQProperty {
    id: root
    function f() {return 42}

    value: f()
}
