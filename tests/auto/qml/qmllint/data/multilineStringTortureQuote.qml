import QtQml

QtObject {
    property string quote: "
    quote: \" \\\" \\\\\"
    ticks: ` \` \\\` \\\`
    singleTicks: ' \' \\' \\\'
    expression: \${expr} \${expr} \\\${expr} \\\${expr}
    "
}
