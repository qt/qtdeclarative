import QtQml 2.0
import selfreferencingsingletonmodule 1.0
pragma Singleton
QtObject {
    property SelfReferencingSingleton self
    property int dummy: 42
}
