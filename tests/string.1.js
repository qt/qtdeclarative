
var s = String(123) + 1
print(s)

var s2 = new String(123)
print(s2, s2.toString, s2.toString())

var s3 = String.prototype.constructor(321)
print(s3)

var s4 = new String.prototype.constructor(321)
print(s4)

