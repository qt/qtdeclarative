import QtQuick

LocallyImported {
    property string derivedMessage: "derived"
    message: "derived.message"

    baseObject: Text {
        text: "derived.baseObject"
    }

    Text {
        text: "derived.child[0]"
    }

    Rectangle {
        property string text: "derived.child[1]"
    }

    HelloWorld {
        property string text: "derived.child[2]"
    }
}
