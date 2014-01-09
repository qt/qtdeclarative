var max
for (max = 2; max < 100; ++max) {
    var arr = [];
    arr[10000000] = -1
    var i;
    var j;
    for (i = 0; i < max; ++i)
        arr[i] = i;
    for (i = 1; i < max; i += 2) {
        delete arr[i];
        for (j = 0; j < max; ++j) {
            if (j <= i && (j %2)) {
                if (arr[j] != undefined)
                    throw "err1"
            } else {
                if (arr[j] != j)
                    throw "err2"
            }
        }
    }
}
