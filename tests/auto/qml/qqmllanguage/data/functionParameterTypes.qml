import QtQuick 2.0 as QQ
import QtQml 2.0 as Core
QQ.Item {
    id: root
    function returnItem() : Core.QtObject { return root; }
    function takeString(arg: string) { return arg; }
}
