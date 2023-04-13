import Things
import QtQml

QtObject {
    property MediaPlayerStateMachine m: MediaPlayerStateMachine {
        id: stateMachine
    }

    objectName: stateMachine.objectName
}
