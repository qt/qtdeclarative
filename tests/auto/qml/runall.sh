#!/bin/bash
# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

if [ "$(uname)" = Linux ]
then
    Xnest :7 2>/dev/null &
    sleep 1
    trap "kill $!" EXIT
    export DISPLAY=:7
    export LANG=en_US
    kwin 2>/dev/null &
    sleep 1
fi

function filter
{
    exe=$1
    skip=0
    while read line
    do
        if [ $skip != 0 ]
        then
            let skip=skip-1
        else
            case "$line" in
            make*Error) echo "$line";;
            make*Stop) echo "$line";;
            /*/bin/make*) ;;
            make*) ;;
            install*) ;;
            QQmlDebugServer:*Waiting*) ;;
            QQmlDebugServer:*Connection*) ;;
            */qmake*) ;;
            */bin/moc*) ;;
            *targ.debug*) ;;
            g++*) ;;
            cd*) ;;
            XFAIL*) skip=1;;
            SKIP*) skip=1;;
            PASS*) ;;
            QDEBUG*) ;;
            Makefile*) ;;
            Config*) ;;
            Totals*) ;;
            \**) ;;
            ./*) ;;
            *tst_*) echo "$line" ;;
            *) echo "$exe: $line"
            esac
        fi
    done
}

make -k -j1 install 2>&1 | filter build
for exe in $(make install | sed -n 's/^install .* "\([^"]*qt4\/tst_[^"]*\)".*/\1/p')
do
    echo $exe
    $exe 2>&1 | filter $exe
done

