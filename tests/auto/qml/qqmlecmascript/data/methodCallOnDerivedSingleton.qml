import Qt.test
import QtQml

QtObject {
    Component.onCompleted: SingletonInheritanceTest.trackPage("test", {x: 42})
}
