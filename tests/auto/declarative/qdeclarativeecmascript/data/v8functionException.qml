import QtQuick 2.0

// This test uses a multi-line string which has \r-terminated
// string fragments.  The expression rewriter deliberately doesn't
// handle \r-terminated string fragments (see QTBUG-24064) and thus
// this test ensures that we don't crash when the client attempts
// to invoke a non-compiled dynamic slot.

Item {
    id: root

    function dynamicSlot() {
        var someString = "Hello,        this is a        multiline string";
    }
}
