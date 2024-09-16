import QtQml

QtObject {
    enum Foo {
        Bar,
        Baz
    }

    property var eF: StoreMetaEnum.Foo
    property int bar: eF.Bar
    property int baz: eF.Baz
}
