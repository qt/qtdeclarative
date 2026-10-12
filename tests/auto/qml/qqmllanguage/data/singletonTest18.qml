import QtQuick 2.0
import "singleton"
import qqmllanguage.Test 1.0

Item {
    property var qmlSingleton: SingletonType
    property var jsSingleton: MyQJSValueQObjectSingleton
    property var cppSingleton: MyTypeObjectSingleton
}
