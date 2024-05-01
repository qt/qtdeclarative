// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.util.Log;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.HashSet;

public abstract class QtQmlComponent
{
    private final static String TAG = "QtQmlComponent";

    private WeakReference<QtQuickView> m_viewReference;
    private QtQmlStatusChangeListener m_statusChangeListener = null;
    private HashSet<Integer> m_signalListenerIds = new HashSet<>();

    protected abstract String getLibraryName();
    protected abstract String getModuleName();
    protected abstract String getFilePath();

    public void setStatusChangeListener(QtQmlStatusChangeListener listener)
    {
        m_statusChangeListener = listener;
        QtQuickView view = getQuickView();
        if (view != null)
            view.setStatusChangeListener(listener);
    }

    protected QtQuickView getQuickView()
    {
        if (m_viewReference != null)
            return m_viewReference.get();
        return null;
    }

    protected boolean isViewAttached() { return getQuickView() != null; }

    protected void attachView(QtQuickView view)
    {
        m_viewReference = new WeakReference<>(view);
        if (view != null)
            view.setStatusChangeListener(m_statusChangeListener);
    }

    protected void detachView()
    {
        QtQuickView view = getQuickView();
        if (view != null) {
            for (int signalListenerId : m_signalListenerIds)
                view.disconnectSignalListener(signalListenerId);

            view.setStatusChangeListener(null);
            m_viewReference.clear();
            if (m_statusChangeListener != null)
                m_statusChangeListener.onStatusChanged(QtQmlStatus.NULL);
        }
    }

    protected HashMap<String, Object> attributes() { return new HashMap<>(); }

    protected void setProperty(String propertyName, Object value)
    {
        QtQuickView view = getQuickView();
        if (view == null) {
            Log.w(TAG, "Cannot set property as the QQmlComponent is not loaded in a QtQuickView.");
            return;
        }
        view.setProperty(propertyName, value);
    }

    protected <T> T getProperty(String propertyName)
    {
        QtQuickView view = getQuickView();
        if (view == null) {
            Log.w(TAG, "Cannot get property as the QQmlComponent is not loaded in a QtQuickView.");
            return null;
        }
        return view.<T>getProperty(propertyName);
    }

    protected <T> int connectSignalListener(String signalName, Class<T> argType,
                                            QtSignalListener<T> listener)
    {
        QtQuickView view = getQuickView();
        if (view == null) {
            Log.w(TAG,
                  "Cannot connect signal listener as the QQmlComponent is not loaded in a "
                          + "QtQuickView.");
            return -1;
        }
        int signalListenerId = view.connectSignalListener(signalName, argType, listener);
        m_signalListenerIds.add(signalListenerId);
        return signalListenerId;
    }

    public boolean disconnectSignalListener(int signalListenerId)
    {
        QtQuickView view = getQuickView();
        if (view == null) {
            Log.w(TAG,
                  "Cannot disconnect signal listener as the QQmlComponent is not loaded in a "
                          + "QtQuickView.");
            return false;
        }
        m_signalListenerIds.remove(signalListenerId);
        return view.disconnectSignalListener(signalListenerId);
    }
}
