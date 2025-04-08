import org.qtproject.AutoTestQmlNestedPluginType.Nested 1.0
import org.qtproject.AutoTestQmlNestedPluginType 1.0
import QtQml

MyNestedPluginType {
    property Conflict conflict: Conflict {}
    Component.onCompleted: console.log(conflict.value)
}
