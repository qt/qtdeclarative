pragma Singleton

import QtQml 2.0

// This causes a duplicate id error on purpose
QtObject {
    id: foo

    QtObject {
        id: foo
    }

    function test() { return "Foobar"; }
}
