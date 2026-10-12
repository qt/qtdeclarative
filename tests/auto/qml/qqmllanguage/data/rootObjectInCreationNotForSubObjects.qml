import QtQml 2.0
import qqmllanguage.Test 1.0
RootObjectInCreationTester {
    subObject: SubType {
        property int testValue: 42;
    }
}
