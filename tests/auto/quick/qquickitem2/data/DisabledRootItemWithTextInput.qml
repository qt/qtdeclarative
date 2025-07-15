import QtQuick

Window {
    id: root
    visible: true
    contentItem.enabled: false

    TextInput {
        text: "text field " + enabled
    }
}

