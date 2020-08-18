import QtQuick 2.15

Rectangle {
    id: messenger

    signal send( string person, string notice)

    onSend: function(person, notice) {
        console.log("For " + person + ", the notice is: " + notice)
    }

    Component.onCompleted: messenger.send("Tom", "the door is ajar.")
}
