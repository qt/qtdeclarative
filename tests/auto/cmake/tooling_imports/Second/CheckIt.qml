import QtQml 2.0
import First

QtObject {
    property DoSomething otherModule: DoSomething {}
    property int dummy: otherModule.success + 13
}
