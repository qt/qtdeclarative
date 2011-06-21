import QtQuick 2.0


Rectangle {
    id: root

    property Bubble messageBubble: initialBubble
    property ListModel script: Conversation {}
    property int scriptIndex: 0
    property string sender: "Me"
    property int messageCounter: 0

    function send() {
        messageBubble.sending = true
        visualModel.insert(visualModel.count, messageBubble)
        messageBubble = bubbleComponent.createObject(composer, { "messageId": ++messageCounter } )
    }

    width: 480; height: 640

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#B0E2FF" }
        GradientStop { position: 1.0; color: "#87CEFA" }
    }

    Component {
        id: bubbleComponent

        ComposerBubble {
            sender: root.sender
        }
    }

    ListView {
        id: messageView
        anchors { left: parent.left; top: parent.top; right: parent.right; bottom: composer.top; margins: 2 }
        spacing: 5

        add: Transition {
            NumberAnimation { properties: "y"; easing.type: Easing.InOutQuad; duration: 1500 }
        }

        model: VisualItemModel {
            id: visualModel

            VisualDataModel {
                model: ListModel {
                    id: messageModel
                }

                delegate: Bubble {
                    y: -height
                    outbound: model.outbound
                    sender: model.sender
                    message:  model.message
                }
            }

            onItemDataInserted: {
                for (var i = 0; i < indexes.length; ++i) {
                    for (var j = indexes[i].start; j < indexes[i].end; ++j) {
                        var message = messageModel.get(visualModel.getItemInfo(j).index)
                        if (!message.outbound)
                            continue
                        for (var k = 0; k < visualModel.children.length; ++k) {
                            var item = visualModel.children[k]
                            if (item.messageId != message.messageId)
                                continue
                            visualModel.replace(j, item)
                            break
                        }
                    }

                }
            }

        }
    }

    Timer {
        interval: 10000
        repeat: true
        running: true
        onTriggered: {
            messageModel.append(script.get(scriptIndex))
            scriptIndex = (scriptIndex + 1) % script.count
            interval = Math.random() * 30000
        }
    }

    Item {
        id: composer

        height: messageBubble.height
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 2 }

        Behavior on height {
            NumberAnimation { duration: 500 }
        }

        ComposerBubble {
            id: initialBubble

            sender: root.sender
            messageId: 0
        }

        Rectangle {
            id: sendButton

            anchors {
                left: messageBubble.right; right: parent.right; top: parent.top; bottom: parent.bottom
                leftMargin: 2; rightMargin: 1; bottomMargin: 1
            }
            radius: 6

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#A2CD5A" }
                GradientStop { position: 1.0; color: sendArea.pressed ? "#556B2F" : "#6E8B3D" }
            }

            Text {
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: 14
                text: "Send"
            }

            MouseArea {
                id: sendArea
                anchors.fill: parent
                onClicked: root.send()
            }
        }
    }
}
