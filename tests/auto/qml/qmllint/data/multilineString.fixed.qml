import QtQml

QtObject {
    property string multipart: `Foo
multiline\`
string` + `another\`
part
of it`;

    property string quote: `
quote: " \\" \\\\"
ticks: \` \` \\\` \\\`
singleTicks: ' \' \\' \\\'
expression: \${expr} \${expr} \\\${expr} \\\${expr}`

    property string tick: `
quote: " \" \\" \\\"
ticks: \` \` \\\` \\\`
singleTicks: ' \\' \\\\'
expression: \${expr} \${expr} \\\${expr} \\\${expr}`
}
