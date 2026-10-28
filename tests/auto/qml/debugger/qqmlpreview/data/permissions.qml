import QtQml
import PermissionReporter

QtObject {
    Component.onCompleted: PermissionReporter.report(Qt.resolvedUrl("permissions.qml"),
                                                     Qt.resolvedUrl("."))
}
