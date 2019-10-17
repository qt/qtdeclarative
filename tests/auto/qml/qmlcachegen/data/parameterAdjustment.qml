import QtQml 2.12

QtObject {
    signal testSignal(string a, int b, string c, bool d, bool e, real f, real g, bool h, int i, int j, string k, int l, string m, string n)
    onTestSignal: {}
    Component.onCompleted: testSignal("a", 1, "b", true, true, 0.1, 0.1, true, 1, 1, "a", 1, "a", "a")
}
