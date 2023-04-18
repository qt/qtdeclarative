import QML

QtObject {
    id: self

    property QtObject l1
    property QtObject l2
    property QtObject l3

    function jsArray() { return [l1, l2, l3, l1, l2, l3] }

    property string jsArrayToString: jsArray().toString()
    property bool jsArrayIncludes: jsArray().includes(l3)
    property string jsArrayJoin: jsArray().join()
    property int jsArrayIndexOf: jsArray().indexOf(l2)
    property int jsArrayLastIndexOf: jsArray().lastIndexOf(l3)
}
