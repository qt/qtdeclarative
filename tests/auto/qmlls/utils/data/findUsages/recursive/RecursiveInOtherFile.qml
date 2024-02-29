import QtQuick

Item {
    function recursive(n: int): int {
        if (n > 3)
            return 1 + recursive(recursive(x-1) + recursive(x-2) - recursive(x-3));
        else
            return recursive(0);
    }
}
