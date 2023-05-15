pragma Strict
import QML

QtObject {
    id: self

    property QtObject l1: QtObject { objectName: "klaus" }
    property QtObject l2: QtObject { function toString(): string { return "teil" } }
    property QtObject l3: QtObject { }

    function jsArray() : list<var> { return [l1, l2, l3, l1, l2, l3] }
    property list<QtObject> listProperty: [l1, l2, l3, l1, l2, l3]

    property string jsArrayToString: jsArray().toString()
    property string listPropertyToString: listProperty.toString()

    property bool listPropertyIncludes: listProperty.includes(l3)
    property bool jsArrayIncludes: jsArray().includes(l3)

    property string listPropertyJoin: listProperty.join()
    property string jsArrayJoin: jsArray().join()

    property int listPropertyIndexOf: listProperty.indexOf(l2)
    property int jsArrayIndexOf: jsArray().indexOf(l2)

    property int listPropertyLastIndexOf: listProperty.lastIndexOf(l3)
    property int jsArrayLastIndexOf: jsArray().lastIndexOf(l3)
}
