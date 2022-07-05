import QML

QtObject {
    id: self

    required property real i
    required property real j
    required property real k

    property QtObject l1: QtObject { objectName: "klaus" }
    property QtObject l2: QtObject { function toString(): string { return "teil" } }
    property QtObject l3: QtObject { }

    function jsArray() { return [l1, l2, l3] }
    property list<QtObject> listProperty: [l1, l2, l3]

    property list<QtObject> listPropertyCopyWithin: listProperty
    property list<QtObject> jsArrayCopyWithin: jsArray().copyWithin(i, j, k)

    property list<QtObject> listPropertyFill: listProperty
    property list<QtObject> jsArrayFill: jsArray().fill(self, i, Math.min(j, 1024))

    property list<QtObject> listPropertySlice: listProperty.slice(i, j)
    property list<QtObject> jsArraySlice: jsArray().slice(i, j)

    property list<QtObject> listPropertySplice: listProperty
    property list<QtObject> listPropertySpliced

    property list<QtObject> jsArraySplice
    property list<QtObject> jsArraySpliced

    property int listPropertyIndexOf: listProperty.indexOf(l2, i)
    property int jsArrayIndexOf: jsArray().indexOf(l2, i)

    property int listPropertyLastIndexOf: listProperty.lastIndexOf(l3, i)
    property int jsArrayLastIndexOf: jsArray().lastIndexOf(l3, i)

    Component.onCompleted: {
        listPropertyCopyWithin.copyWithin(i, j, k);
        listPropertyFill.fill(self, i, Math.min(j, 1024));

        listPropertySpliced = listPropertySplice.splice(i, j, self, self, self);
        var a = jsArray();
        jsArraySpliced = a.splice(i, j, self, self, self);
        jsArraySplice = a;
    }
}
