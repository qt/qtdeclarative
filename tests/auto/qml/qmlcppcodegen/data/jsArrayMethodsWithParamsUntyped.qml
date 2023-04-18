import QML

QtObject {
    id: self

    required property int i
    required property int j
    required property int k

    property QtObject l1
    property QtObject l2
    property QtObject l3

    function jsArray() { return [l1, l2, l3, l1, l2, l3] }
    property var jsArraySlice: jsArray().slice(i, j)
    property int jsArrayIndexOf: jsArray().indexOf(l2, i)
    property int jsArrayLastIndexOf: jsArray().lastIndexOf(l3, i)
}
