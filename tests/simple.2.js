
function main()
{
    var a = 1
    var b = 2
    var c = 10
    var d = 100

    for (var i = 0; i < 1000000; i = i + 1) {
        if (a == 1)
            d = d + a + b * c
        else
            d = 321
    }

    print("the result is", d)
}

main()


print("Object.prototype", Object.prototype)
print("Boolean.prototype", Boolean.prototype)
print("Number.prototype", Number.prototype)
print("String.prototype", String.prototype)
print("Date.prototype", Date.prototype)
print("Array.prototype", Array.prototype)
print("Function.prototype", Function.prototype)


print("typeof Object.prototype", typeof(Object.prototype))
print("typeof Boolean.prototype", typeof(Boolean.prototype))
print("typeof Number.prototype", typeof(Number.prototype))
print("typeof String.prototype", typeof(String.prototype))
print("typeof Date.prototype", typeof(Date.prototype))
print("typeof Array.prototype", typeof(Array.prototype))
print("typeof Function.prototype", typeof(Function.prototype))
