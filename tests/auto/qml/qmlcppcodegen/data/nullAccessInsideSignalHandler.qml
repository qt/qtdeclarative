import QtQuick

Item {
    id: root
    visible: true

    property var speaker
    signal say_hello()

    Component{
        id: speakerComp
        Text {
            text: "HELLO"
            function say_hello() {
                console.log(text)
            }
        }
    }

    Timer {
        interval: 1; running: true; repeat: false
        onTriggered: root.say_hello();
    }

    Component.onCompleted:
    {
        root.speaker = speakerComp.createObject(root);

        root.say_hello.connect(root.speaker.say_hello);

        root.speaker.destroy();
    }
}
