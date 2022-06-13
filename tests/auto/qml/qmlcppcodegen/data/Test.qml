pragma Strict
import TestTypes 1.0

CppBaseClass {
    id: self

    enum EE {
        AA, BB, CC
    }

    property int foo: 1 + 2
    property int ppp: 4

    // constant, binding will be removed.
    cppProp: 3 + 4

    // An actual binding. Can't be removed because cppProp may be manually set.
    cppProp2: cppProp * 2

    property int a: boo[0]
    function incA() : void {
        self.a = self.boo[1];
    }

    property real b: hoo[0]
    function incB() : void {
        self.b = self.hoo[1];
    }

}
