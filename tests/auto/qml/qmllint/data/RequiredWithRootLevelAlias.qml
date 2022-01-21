import QtQuick

Item {
    property alias requiredAlias: nested.foo
    Item {
        id: nested
        required property string foo
    }
}
