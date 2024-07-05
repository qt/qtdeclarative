import "./module.mjs" as MJ
import QtQml 2.15

QtObject {
    id: root
    required property string prefix
    property bool ok: false
    Component.onCompleted: {
        root.ok = MJ.withProp(root, prefix) == 42
    }
}
