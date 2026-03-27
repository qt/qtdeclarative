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

    property real safe32: LargeEnum.Flag.Safe32
    property real safe53: LargeEnum.Flag.Safe53
    property real safe63: LargeEnum.Flag.Safe63
    property real broken: LargeEnum.Flag.Broken

    property real safe32FromProperty: LargeEnum.safe32
    property real safe53FromProperty: LargeEnum.safe53
    property real safe63FromProperty: LargeEnum.safe63
    property real brokenFromProperty: LargeEnum.broken
}
