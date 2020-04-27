import QtQml 2.0
import Test.PropertyCache 1.0

GadgetUser {
    baseString: base.stringValue()
    derivedString: derived.stringValue()
}
