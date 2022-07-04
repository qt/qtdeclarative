pragma Strict
import QML

QtObject {
    property Component c: Component {
        id: cc
        QtObject {}
    }
        
    property bool ready: cc.status == Component.Ready
}
