
function fact1(n) {
    if (n > 0)
        return n * fact1(n - 1);
    else
        return 1
}

function fact2(n) {
    return n > 0 ? n * fact2(n - 1) : 1
}

print("fact(12) = ", fact1(12))
print("fact(12) = ", fact2(12))

