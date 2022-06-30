pragma Strict
import QML

QtObject {
    function test() : int {
        var i = 0, x;
        while (i == 0) {
            x = 1;
            i = 1;
        }

        if (i == 1)
            return x;

        return 0
    }

    function produceUndefined1() : string {
        var x;
        return x + "";
    }

    function produceUndefined2() : string {
        var i = 0, x;
        while (i == 1) {
            x = 1;
            i = 1;
        }

        if (i == 0)
            return x + "";

        return 0
    }
}
