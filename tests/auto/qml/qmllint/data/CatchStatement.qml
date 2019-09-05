import QtQml 2.12

QtObject {
    function f() {
        try {} catch(err) {}
        console.log(err);
    }
}
