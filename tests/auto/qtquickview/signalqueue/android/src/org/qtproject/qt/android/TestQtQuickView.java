// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;

class TestQtQuickView extends QtQuickView
{
    native void onConnectSignalListenerCalled(String signalName, Class<?>[] argTypes,
                                              Object listener, int id);

    TestQtQuickView(Context ctx) {
        super(ctx);
    }

    @Override
    boolean connectSignalListener(String signalName, Class<?>[] argTypes, Object listener,
                                  int id)
    {
        onConnectSignalListenerCalled(signalName, argTypes, listener, id);
        return true;
    }
}
