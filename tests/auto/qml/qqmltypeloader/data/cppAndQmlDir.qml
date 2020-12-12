import QtQml 2.0
import declarative.import.for.typeloader.test 3.2

DeclarativeTestType {
    Component.onCompleted: console.log(Singleton, Singleton.objectName)
    objectName: Singleton.objectName
}
