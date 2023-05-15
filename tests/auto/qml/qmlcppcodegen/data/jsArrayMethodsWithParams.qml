pragma Strict
import QML

QtObject {
    id: self

    required property int i
    required property int j
    required property int k

    property QtObject l1: QtObject { objectName: "klaus" }
    property QtObject l2: QtObject { function toString(): string { return "teil" } }
    property QtObject l3: QtObject { }

    function jsArray() : list<var> { return [l1, l2, l3, l1, l2, l3] }
    property list<QtObject> listProperty: [l1, l2, l3, l1, l2, l3]

    property list<QtObject> listPropertySlice: listProperty.slice(i, j)
    property list<var> jsArraySlice: jsArray().slice(i, j)

    property int listPropertyIndexOf: listProperty.indexOf(l2, i)
    property int jsArrayIndexOf: jsArray().indexOf(l2, i)

    property int listPropertyLastIndexOf: listProperty.lastIndexOf(l3, i)
    property int jsArrayLastIndexOf: jsArray().lastIndexOf(l3, i)
}
