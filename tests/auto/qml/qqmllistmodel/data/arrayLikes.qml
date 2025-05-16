import QtQml
import Test

VariantListProvider {
    id: testItem

    property ListModel listMode: ListModel {
        id: testModel
    }

    function getSomeJavaScriptArray(count) {
        var list = [];
        for (var i = 0; i < count; ++i)
            list.push({ r: i, s: String(i) + "t" });
        return { a: 3, b: list };
    }

    property int count: testModel.count
    property int a
    property int r
    property string s

    function setVariantList() {
        testModel.set(0, getSomeData(10));
        testModel.set(0, getSomeData(10));
        a = testModel.get(0).a
        r = testModel.get(0).b.get(1).r
        s = testModel.get(0).b.get(2).s
    }

    function setJavaScriptArray() {
        testModel.set(0, getSomeJavaScriptArray(10));
        testModel.set(0, getSomeJavaScriptArray(10));
        a = testModel.get(0).a
        r = testModel.get(0).b.get(1).r
        s = testModel.get(0).b.get(2).s
    }
}
