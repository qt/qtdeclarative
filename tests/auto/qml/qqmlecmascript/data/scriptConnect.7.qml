import Qt.test
import QtQml

QtObject {
    readonly property Sender s: Sender {id: sender}
    readonly property Receiver r: Receiver {id: receiver}
    Component.onCompleted: () => {
        sender.sig1.connect(receiver.slot1)
        sender.sig1()
    }
}
