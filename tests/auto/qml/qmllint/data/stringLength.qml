import QtQuick 2.15

TextInput {
    id: textInput

    Component.onCompleted: {
        console.log("text.length", textInput.text.length);
    }
}
