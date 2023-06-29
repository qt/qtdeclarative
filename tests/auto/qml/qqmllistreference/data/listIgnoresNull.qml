import QtQml
import Test

TestItem {
    id: testItem

    Component.onCompleted : {
        testItem.data.push(null);
        testItem.data.length = 5;
        testItem.data.unshift(null);
    }
}
