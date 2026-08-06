import QtQml
import bindable 1.0

QtObject {
    id: root
    property int trigger: 1
    property QtObject inner: WithBinding {
        id: innerObject
        BindableInterceptor on prop {}
    }
    property alias aliasProp: innerObject.prop
}
