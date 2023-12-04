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
    native void setRootObjectProperty(long windowReference, String propertyName, Object value);
    native Object getRootObjectProperty(long windowReference, String propertyName);

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

    public void setProperty(String propertyName, Object value)
    {
        setRootObjectProperty(windowReference(), propertyName, value);
    }

    // getRootObjectProperty always returns a primitive type or an Object
    // so it is safe to suppress the unchecked warning
    @SuppressWarnings("unchecked")
    public <T extends Object> T getProperty(String propertyName)
    {
        return (T)getRootObjectProperty(windowReference(), propertyName);
    }
}
