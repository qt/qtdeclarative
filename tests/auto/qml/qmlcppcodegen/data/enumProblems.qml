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

    property int a: FooFactory.B
    property int b: f.t8
    property int c: FooFactory.D
    property int d: f.t16
}
