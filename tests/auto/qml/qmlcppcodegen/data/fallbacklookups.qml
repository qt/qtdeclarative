import TestTypes
import QtQml

DynamicMeta {
    id: self

    function getSingleton(): QtObject {
        return DynamicMetaSingleton.itself
    }

    function withContext(): int {
        foo = 93;
        objectName = "aa" + foo;
        return bar(4);
    }

    function withId(): int {
        self.foo = 94;
        self.objectName = "bb" + foo;
        return self.bar(5);
    }

    function withSingleton(): int {
        DynamicMetaSingleton.foo = 95;
        DynamicMetaSingleton.objectName = "cc" + DynamicMetaSingleton.foo;
        return DynamicMetaSingleton.bar(6);
    }

    function withProperty(): int {
        DynamicMetaSingleton.itself.foo = 96;
        DynamicMetaSingleton.itself.objectName = "dd" + DynamicMetaSingleton.itself.foo;
        return DynamicMetaSingleton.itself.bar(7);
    }
}
