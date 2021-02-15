#!/bin/bash

REPEAT_TIMES=100

to_repeat=$(cat $1)

echo -e "import QtQuick 2.15\nimport Test 1.0\n\nMyQmlObject {\nid: qmlObject\nresult: ###\n\n" > $2

for x in $(seq 1 $REPEAT_TIMES); do
    echo "$to_repeat" >> $2;
done

echo -e "}\n" >> $2
