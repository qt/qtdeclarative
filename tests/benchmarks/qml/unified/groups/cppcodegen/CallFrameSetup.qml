import QtQml

QtObject {
    function noop(): int { return 3 }
    function comp(): Component { return something }

    property Component c: Component {
        id: something
        QtObject {}
    }
}
