import QtQml 2.0
import First
import Second
import Test   // Self import

QtObject {
    property DoSomething firstModule: DoSomething{}
    property CheckIt secondModule: CheckIt{}
    property int result: firstModule.success + secondModule.dummy
    property Ttt mine: Ttt{}
}
