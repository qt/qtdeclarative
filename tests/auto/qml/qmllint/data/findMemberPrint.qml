import QtQml

import TestTypes

QtObject {
    property var foooooooo: Foo {
        id: foo
    }

    function f(): void {
        foo.print()
    }
}
