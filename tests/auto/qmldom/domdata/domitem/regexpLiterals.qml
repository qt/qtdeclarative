import QtQuick

Item {
    function f() {
        let noFlag = /HelloWorld/;
        let withFlag = /H?ello.*[^s]+/g;
    }

}
