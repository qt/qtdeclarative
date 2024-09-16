// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.util.Log;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.HashSet;

/**
 * @Since 6.8
 *
 * The QtQuickViewContent represents a QML component that can be loaded by a QtQuickView instance
 * This abstract class should be extended to be used by a QtQuickView. It provides QtQuickView with
 * essential information to load the QML component it represents.
 * It also offers convenient methods for seamless interaction with the QtQuickView that loads it.
 **/
public abstract class QtQuickViewContent
{
    private final static String TAG = "QtQuickViewContent";

    private WeakReference<QtQuickView> m_viewReference;
    private QtQmlStatusChangeListener m_statusChangeListener = null;
    private HashSet<Integer> m_signalListenerIds = new HashSet<>();

    /**
     * Implement this to return the library name that this component belongs to.
     **/
    protected abstract String getLibraryName();
    /**
     * Implement this to return the module name that this component belongs to.
     **/
    protected abstract String getModuleName();
    /**
     * Implement this to return the qrc (Qt Resource) path of this QML component.
     **/
    protected abstract String getFilePath();

    /**
     * Sets a StatusChangeListener to listen to status changes.
     * <p>
     * @param listener an instance of a StatusChangeListener interface
     **/
    public void setStatusChangeListener(QtQmlStatusChangeListener listener)
    {
        m_statusChangeListener = listener;
        QtQuickView view = getQuickView();
        if (view != null)
            view.setStatusChangeListener(listener);
    }

    /**
     * Gets the QtQuickView instance that has loaded this component.
     * <p>
     * @return Returns an instance of QtQuickView or null if this component is not loaded by any
     * QtQuickView.
     **/
    protected QtQuickView getQuickView()
    {
        if (m_viewReference != null)
            return m_viewReference.get();
        return null;
    }

    /**
     * Checks if this is currently attached to a QtQuickView instance
     * <p>
     * @return Returns true if this is attached to a QtQuickView instance, otherwise, returns false.
     **/
    protected boolean isViewAttached() { return getQuickView() != null; }

    /**
     * Attaches this to a QtQuickView instance.
     **/
    protected void attachView(QtQuickView view)
    {
        m_viewReference = new WeakReference<>(view);
        if (view != null)
            view.setStatusChangeListener(m_statusChangeListener);
    }

    /**
     * Detaches this from the QtQuickView to which it has previously been attached. A call to this
     * method will disconnect all signal listeners that have been connected before.
     **/
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

    /**
     * Implement this to return more information about the QML Component.
     * Default implementation returns an empty HashMap.
     **/
    protected HashMap<String, Object> attributes() { return new HashMap<>(); }

    /**
     * Sets the value of an existing property on the QML component if it has already been attached
     * and loaded by a QtQuickView instance. The supported types are
     * {@link java.lang.Integer}, {@link java.lang.Double}, {@link java.lang.Float},
     * {@link java.lang.Boolean} and {@link java.lang.String}. These types get converted to their
     * corresponding QML types int, double/float, bool, and string. This function does not add
     * properties to the QML root object if they do not exist but prints a warning.
     * <p>
     * @param propertyName the name of the existing QML property to set the value of
     * @param value        the value to set the property to QML's int, double/float,
                           bool or string
     * @see <a href="https://doc.qt.io/qt-6/qml-int.html">QML int</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-double.html">QML double/float</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-bool.html">QML bool</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-string.html">QML string</a>
     **/
    protected void setProperty(String propertyName, Object value)
    {
        QtQuickView view = getQuickView();
        if (view == null) {
            Log.w(TAG, "Cannot set property as the QQmlComponent is not loaded in a QtQuickView.");
            return;
        }
        view.setProperty(propertyName, value);
    }

    /**
     * Gets the value of an existing property of the QML component if it has already been attached
     * and loaded by a QtQuickView instance. The supported types are
     * {@link java.lang.Integer}, {@link java.lang.Double}, {@link java.lang.Float},
     * {@link java.lang.Boolean} and {@link java.lang.String}. These types get converted to their
     * corresponding QML types int, double/float, bool and string. If the property does not
     * exist or the status of the QML component is anything other than
     * {@link QtQuickView#STATUS_READY STATUS_READY}, this function will return null.
     * <p>
     * @param propertyName the name of the existing root object property
     * @throws ClassCastException if the returned type cannot be cast to the requested type.
     * @see <a href="https://doc.qt.io/qt-6/qml-int.html">QML int</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-double.html">QML double/float</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-bool.html">QML bool</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-string.html">QML string</a>
     **/
    protected <T> T getProperty(String propertyName)
    {
        QtQuickView view = getQuickView();
        if (view == null) {
            Log.w(TAG, "Cannot get property as the QQmlComponent is not loaded in a QtQuickView.");
            return null;
        }
        return view.<T>getProperty(propertyName);
    }

    /**
     * Connects a SignalListener to a signal of the QML component if it has already been attached
     * and loaded by a QtQuickView instance.
     * <p>
     * @param signalName the name of the root object signal
     * @param argType    the Class type of the signal argument
     * @param listener   an instance of the QtSignalListener interface
     * @return a connection ID between signal and listener or the existing connection ID if there is
     *         an existing connection between the same signal and listener. Return a negative value
     *         if the signal does not exist on the QML root object.
     **/
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

    /**
     * Disconnects a SignalListener with a given id obtained from
     * {@link QtQuickView#connectSignalListener() connectSignalListener} call, from listening to
     * a signal.
     * <p>
     * @param signalListenerId the connection id
     * @return Returns true if the connection id is valid and has been successfully removed,
     *         otherwise returns false.
     **/
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
