import QtQml

QtObject {
    property Foo theFoo: Foo {}

    function check(foo: Foo) : bool {
        return foo !== null;
    }
}
