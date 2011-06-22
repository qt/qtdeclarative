import QtQuick 2.0

Rectangle {
    id: root

    property Bubble messageBubble: initialBubble
    property ListModel script: Conversation {}
    property int scriptIndex: 0
    property string sender: "Me"
    property int messageCounter: 0

    function send() {
        messageView.positionViewAtEnd()
        visualModel.insert(visualModel.count, messageBubble)
        messageBubble = bubbleComponent.createObject(composer, { "messageId": ++messageCounter } )
        messageView.positionViewAtEnd()
    }

    width: 480; height: 640

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#000000" }
        GradientStop { position: 1.0; color: "#080808" }
    }

    Component {
        id: bubbleComponent

        ComposerBubble {
            sender: root.sender
        }
    }

    ListView {
        id: messageView
        anchors {
            left: parent.left; top: parent.top; right: parent.right; bottom: composer.top
            topMargin: 1; bottomMargin: 2
        }
        spacing: 2

        add: Transition {
            ParentAnimation {
                via: root
                NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 1500 }
            }
        }

        cacheBuffer: 256

        model: VisualItemModel {
            id: visualModel

            VisualDataModel {
                model: ListModel {
                    id: messageModel
                }

                delegate: Bubble {
                    y: -height
                }
            }

            onUpdated: {
                for (var i = 0; i < inserts.length; ++i) {
                    for (var j = inserts[i].start; j < inserts[i].end; ++j) {
                        var message = messageModel.get(visualModel.getItemInfo(j).index)
                        if (!message.outbound)
                            continue
                        for (var k = 0; k < visualModel.children.length; ++k) {
                            var item = visualModel.children[k]
                            if (item.messageId != message.messageId)
                                continue
                            visualModel.replace(j, item)
                            // visualModel.move(item.VisualItemModel.index, j + 1
                            // visualModel.replace(j + 1, j)
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
            var message = script.get(scriptIndex);

            messageModel.append({
                "sender": message.sender,
                "message": message.message,
                "avatar": message.avatar,
                "outbound": false,
                "messageId": -1,
                "time": Qt.formatTime(Date.now())
            })

            scriptIndex = (scriptIndex + 1) % script.count
            interval = Math.random() * 30000
        }
    }

    Item {
        id: composer

        height: messageBubble.height - messageBubble.margin
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }

        ComposerBubble {
            id: initialBubble

            sender: root.sender
            messageId: 0
        }
    }
}
