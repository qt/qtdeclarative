import QtQml

QtObject {
    id: self

    property Component t: Component {
        id: t
        QtObject {}
    }
    property QtObject a: t.createObject()
    property QtObject b: t.createObject(null)
    property QtObject c: t.createObject(self)
    property QtObject d: t.createObject(self, ({objectName: "foo"}))
}
