import "./module.mjs" as MJ
import QtQml 2.15

QtObject {
    id: root
    property bool ok: false
    Component.onCompleted: {
        root.ok = MJ.withProp(root) == 42
    }
}
