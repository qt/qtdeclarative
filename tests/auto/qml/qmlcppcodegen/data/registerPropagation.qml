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
}
