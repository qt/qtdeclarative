import QtQuick

Item {
    component MyIC: Item {}
    MyIC { MyIC{} MyIC {}}
    function f(x: MyIC): MyIC {
        return x;
    }
}
