pragma Strict
import TestTypes
import QtQml

QtObject {
    id: root

    readonly property FooFactory f: FooFactory {}

    property QtObject o: QtObject {
        readonly property FooThing fighter: root.f.get(Foo.Fighter)
        readonly property FooThing bar: root.f.get(Foo.Component)
    }
}
