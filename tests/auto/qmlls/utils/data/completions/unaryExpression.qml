import QtQuick

Item {
    function f(x) {
        -x;
        +x;
        ~x;
        !x;
        typeof x;
        delete x;
        void x;
        x--;
        x++;
        --x;
        ++x;
    }
}
