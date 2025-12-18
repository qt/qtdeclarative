import QtQml
import Other

Other {
    id: root
    Component.onCompleted: {
        // Use an AOT-compiled function depending on the meta object layout
        root.f()
        Qt.quit()
    }
}
