import QtQuick 2.0

Item {
    property bool success: false

    BaseComponent {
        id: foo
        property Text baz: Text { width: 200 }
    }

    Component.onCompleted: success = (foo.bar == '200')
}
