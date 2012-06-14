.pragma library

function twitterName(str)
{
    var s = str.split("(")
    return s[0]
}

function realName(str)
{
    var s = str.split("(")
    return s[1].substring(0, s[1].length-1)
}

function formatDate(date)
{
    var da = new Date(date)
    return da.toDateString()
}
