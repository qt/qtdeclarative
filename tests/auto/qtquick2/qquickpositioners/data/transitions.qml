import QtQuick 2.0

Rectangle {
    id: root
    width: 500
    height: 500

    property int duration: 50

    property int targetTransitionsDone
    property int displaceTransitionsDone

    property var targetTrans_items: new Object()
    property var targetTrans_targetIndexes: new Array()
    property var targetTrans_targetItems: new Array()

    property var displacedTrans_items: new Object()
    property var displacedTrans_targetIndexes: new Array()
    property var displacedTrans_targetItems: new Array()

    // for QDeclarativeListProperty types
    function copyList(propList) {
        var temp = new Array()
        for (var i=0; i<propList.length; i++)
            temp.push(propList[i])
        return temp
    }

    function checkPos(x, y, name) {
        if (Qt.point(x, y) == targetItems_transitionFrom)
            model_targetItems_transitionFrom.addItem(name, "")
        if (Qt.point(x, y) == displacedItems_transitionVia)
            model_displacedItems_transitionVia.addItem(name, "")
    }

    Transition {
        id: targetTransition
        enabled: enableAddTransition

        SequentialAnimation {
            ScriptAction {
                script: {
                    root.targetTrans_items[targetTransition.ViewTransition.item.nameData] = targetTransition.ViewTransition.index
                    root.targetTrans_targetIndexes.push(targetTransition.ViewTransition.targetIndexes)
                    root.targetTrans_targetItems.push(root.copyList(targetTransition.ViewTransition.targetItems))
                }
            }
            ParallelAnimation {
                NumberAnimation { properties: "x"; from: targetItems_transitionFrom.x; duration: root.duration }
                NumberAnimation { properties: "y"; from: targetItems_transitionFrom.y; duration: root.duration }
            }

            ScriptAction { script: root.targetTransitionsDone += 1 }
        }
    }

    Transition {
        id: displaced

        SequentialAnimation {
            ScriptAction {
                script: {
                    root.displacedTrans_items[displaced.ViewTransition.item.nameData] = displaced.ViewTransition.index
                    root.displacedTrans_targetIndexes.push(displaced.ViewTransition.targetIndexes)
                    root.displacedTrans_targetItems.push(root.copyList(displaced.ViewTransition.targetItems))
                }
            }
            ParallelAnimation {
                NumberAnimation { properties: "x"; duration: root.duration; to: displacedItems_transitionVia.x }
                NumberAnimation { properties: "y"; duration: root.duration; to: displacedItems_transitionVia.y }
            }
            NumberAnimation { properties: "x,y"; duration: root.duration }

            ScriptAction { script: root.displaceTransitionsDone += 1 }
        }

    }

    Row {
        objectName: "row"

        property int count: children.length - 1 // omit Repeater

        x: 50; y: 50
        width: 400; height: 400
        Repeater {
            objectName: "repeater"
            Rectangle {
                property string nameData: name
                objectName: "wrapper"
                width: 30 + index*5
                height: 30 + index*5
                border.width: 1
                Column {
                    Text { text: index }
                    Text { objectName: "name"; text: name }
                    Text { text: parent.parent.y }
                }
                onXChanged: root.checkPos(x, y, name)
                onYChanged: root.checkPos(x, y, name)
            }
        }

        add: targetTransition
        move: displaced
    }

    Column {
        objectName: "column"

        property int count: children.length - 1 // omit Repeater

        x: 50; y: 50
        width: 400; height: 400
        Repeater {
            objectName: "repeater"
            Rectangle {
                property string nameData: name
                objectName: "wrapper"
                width: 30 + index*5
                height: 30 + index*5
                border.width: 1
                Column {
                    Text { text: index }
                    Text { objectName: "name"; text: name }
                    Text { text: parent.parent.y }
                }
                onXChanged: root.checkPos(x, y, name)
                onYChanged: root.checkPos(x, y, name)
            }
        }

        add: targetTransition
        move: displaced
    }

    Grid {
        objectName: "grid"

        property int count: children.length - 1 // omit Repeater

        x: 50; y: 50
        width: 400; height: 400
        Repeater {
            objectName: "repeater"
            Rectangle {
                property string nameData: name
                objectName: "wrapper"
                width: 30 + index*5
                height: 30 + index*5
                border.width: 1
                Column {
                    Text { text: index }
                    Text { objectName: "name"; text: name }
                    Text { text: parent.parent.y }
                }

                onXChanged: root.checkPos(x, y, name)
                onYChanged: root.checkPos(x, y, name)
            }
        }

        add: targetTransition
        move: displaced
    }

    Flow {
        objectName: "flow"

        property int count: children.length - 1 // omit Repeater

        x: 50; y: 50
        width: 400; height: 400
        Repeater {
            objectName: "repeater"
            Rectangle {
                property string nameData: name
                objectName: "wrapper"
                width: 30 + index*5
                height: 30 + index*5
                border.width: 1
                Column {
                    Text { text: index }
                    Text { objectName: "name"; text: name }
                    Text { text: parent.parent.x + " " + parent.parent.y }
                }
                onXChanged: root.checkPos(x, y, name)
                onYChanged: root.checkPos(x, y, name)
            }
        }

        add: targetTransition
        move: displaced
    }
}

