import QtQml

QtObject {
    id: self

    property Component t: Component {
        id: t
        QtObject {}
    }
    property QtObject a: t.createObject("foobar")
}
