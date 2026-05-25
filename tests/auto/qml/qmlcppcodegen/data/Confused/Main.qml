import QtQml
import "Test" as T

QtObject {
    Component.onCompleted: T.Test.Print()
}
