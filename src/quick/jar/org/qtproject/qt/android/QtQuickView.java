// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.util.Log;

import java.security.InvalidParameterException;

public class QtQuickView extends QtView {
    private final static String TAG = "QtQuickView";

    @FunctionalInterface
    public interface SignalListener<T>
    {
        void onSignalEmitted(String signalName, T value);
    }

    private String m_qmlUri;
    private String[] m_qmlImportPaths = null;

    native void createQuickView(String qmlUri, int width, int height, long parentWindowReference,
                                String[] qmlImportPaths);
    native void setRootObjectProperty(long windowReference, String propertyName, Object value);
    native Object getRootObjectProperty(long windowReference, String propertyName);
    native int addRootObjectSignalListener(long windowReference, String signalName, Class argType,
                                          Object listener);
    native boolean removeRootObjectSignalListener(long windowReference, int signalListenerId);

    public QtQuickView(Context context, String qmlUri, String appName)
        throws InvalidParameterException {
        this(context, qmlUri, appName, null);
    }

    public QtQuickView(Context context, String qmlUri, String appName, String[] qmlImportPaths)
            throws InvalidParameterException
    {
        super(context, appName);
        if (qmlUri == null || qmlUri.isEmpty()) {
            throw new InvalidParameterException(
                "QtQuickView: argument 'qmlUri' may not be empty or null");
        }
        m_qmlUri = qmlUri;
        m_qmlImportPaths = qmlImportPaths;
    }

    @Override
    protected void createWindow(long parentWindowReference) {
        createQuickView(m_qmlUri, getWidth(), getHeight(), parentWindowReference, m_qmlImportPaths);
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

    public <T> int addSignalListener(String signalName, Class<T> argType,
                                    SignalListener<T> listener)
    {
        int signalListenerId =
                addRootObjectSignalListener(windowReference(), signalName, argType, listener);
        if (signalListenerId < 0) {
            Log.w(TAG, "The signal " + signalName
                     + " does not exist in the root object or the arguments do not match with the listener.");
        }
        return signalListenerId;
    }

    public boolean removeSignalListener(int signalListenerId)
    {
        return removeRootObjectSignalListener(windowReference(), signalListenerId);
    }
}
