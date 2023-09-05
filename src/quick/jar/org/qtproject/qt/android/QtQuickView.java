// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import java.security.InvalidParameterException;

public class QtQuickView extends QtView {
    private String m_qmlUri;

    native void createQuickView(String qmlUri, int width, int height, long parentWindowReference);

    public QtQuickView(Context context, String qmlUri, String appName)
        throws InvalidParameterException {
        super(context, appName);
        if (qmlUri == null || qmlUri.isEmpty()) {
            throw new InvalidParameterException(
                "QtQuickView: argument 'qmlUri' may not be empty or null");
        }

        m_qmlUri = qmlUri;
    }

    @Override
    protected void createWindow(long parentWindowReference) {
        createQuickView(m_qmlUri, getWidth(), getHeight(), parentWindowReference);
    }
}
