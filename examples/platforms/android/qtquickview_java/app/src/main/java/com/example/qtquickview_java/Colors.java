// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qtquickview_java;

import java.util.Arrays;
import java.util.Collections;
import java.util.Stack;

class Colors {
    private final Stack<Integer> recycle;
    private final Stack<Integer> colors;

    public Colors() {
        colors = new Stack<>();
        recycle = new Stack<>();
        recycle.addAll(Arrays.asList(
                        0xff1CB669, 0xff00414A, 0xff27138B,
                        0xffB5C10E, 0xff373F26, 0xffAF93DF,
                        0xff817505
                )
        );
    }

    public String getColor() {
        if (colors.size()==0) {
            while (!recycle.isEmpty())
                colors.push(recycle.pop());
            Collections.shuffle(colors);
        }
        int color = colors.pop();
        recycle.push(color);
        return String.format("#%06X", (0xFFFFFF & color));
    }
}

