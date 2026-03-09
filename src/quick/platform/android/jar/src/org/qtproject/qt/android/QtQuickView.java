// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import org.qtproject.qt.android.QtSignalListener;

import android.content.Context;
import android.util.Log;

import java.lang.IllegalArgumentException;
import java.lang.ref.WeakReference;
import java.security.InvalidParameterException;

/**
 * The QtQuickView class lets you easily add QML content to your Android app as a
 * {@link android.view.View View}. QtQuickView instantiates a QQuickView with a given
 * QML component source URI path and embeds it inside itself. You can add it in your Android app's
 * layout as with any other View. QtQuickView is a good choice when you want to extend your non-Qt
 * Android app with QML content but do not want to make the entire app using the Qt framework.
 * It brings the power of Qt Quick into your Android app, making it possible to use various Qt Quick
 * APIs, in Android Java or Kotlin apps.
 *
 * <b>Known limitation:</b> Only CMake is supported, not qmake.
 * @see <a href="https://doc.qt.io/qt-6/qquickview.html">Qt QQuickView</a>
 **/
public class QtQuickView extends QtView {
    private final static String TAG = "QtQuickView";

    private String m_qmlUri;
    private String[] m_qmlImportPaths = null;
    private QtQmlStatusChangeListener m_statusChangeListener = null;
    private QtQmlStatus m_lastStatus = QtQmlStatus.NULL;
    private boolean m_hasQueuedStatus = false;
    private WeakReference<QtQuickViewContent> m_loadedComponent;
    private QtSignalQueue m_signalQueue = new QtSignalQueue();

    native void createQuickView(String qmlUri, int width, int height, long parentWindowReference,
                                long viewReference, String[] qmlImportPaths);
    native void setRootObjectProperty(long windowReference, String propertyName, Object value);
    native Object getRootObjectProperty(long windowReference, String propertyName);
    native boolean addRootObjectSignalListener(long windowReference, String signalName,
                                               Class<?>[] argTypes, Object listener, int id);
    native boolean removeRootObjectSignalListener(long windowReference, int signalListenerId);
    native void invokeMethod(long windowReference, String methodName, Object[] params);

    /**
     * Creates a QtQuickView to load and view a QML component. Instantiating a QtQuickView will load
     * the Qt libraries, including the app library specified by <code>appName</code>. Then it
     * creates a QQuickView that loads the QML source specified by <code>qmlUri</code>.
     *
     * @param context the parent Context
     * @param qmlUri  the URI of the main QML file
     * @param appName the name of the Qt app library to load and start. This corresponds to the
     *                target name set in Qt app's CMakeLists.txt
     * @throws InvalidParameterException if either qmlUri or appName is empty or null
     * @see <a href="https://doc.qt.io/qt-6/qquickview.html">Qt QQuickView</a>
     **/
    public QtQuickView(Context context, String qmlUri, String appName)
        throws InvalidParameterException {
        this(context, qmlUri, appName, null);
    }

    /**
     * Creates a QtQuickView to load and view a QML component. Instantiating a QtQuickView will load
     * the Qt libraries, including the app library specified by appName. Then it creates a
     * QQuickView that loads the QML source specified by qmlUri. This overload accepts an array of
     * strings in the case where the QML application should load QML modules from custom paths.
     *
     * @param context        the parent Context
     * @param qmlUri         the URI of the main QML file
     * @param appName        the name of the Qt app library to load and start. This corresponds to
     *                       the target name set in the Qt app's CMakeLists.txt
     * @param qmlImportPaths an array of strings for additional import paths to be passed to
                             QQmlEngine, or null if additional import paths are not required
     * @throws InvalidParameterException if either qmlUri or appName is empty or null
     * @see <a href="https://doc.qt.io/qt-6/qqmlengine.html">Qt QQmlEngine</a>
     **/
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

    /**
     * Creates a QtQuickView that can later load and view a QML component by calling
     * {@link #loadContent(QtQuickViewContent, String[])}
     *
     * @param context the parent Context
     **/
    public QtQuickView(Context context)
    {
        super(context);
    }

    /**
     * Loads a QML content represented by a QtQuickViewContent. The library name and the qrc path
     * of the QML content will be extracted from the QtQuickViewContent to load the QML content.
     * This overload accepts an array of strings in the case where the QML content should load
     * QML modules from custom paths.
     *
     * @param qmlContent   an instance of an object that extends QtQuickViewContent
     * @param qmlImportPaths an array of strings for additional import paths to be passed to
     *                       QQmlEngine, or null if additional import paths are not required
     * @throws InvalidParameterException if QtQuickViewContent does not contain valid information
     * about the module name, and the qrc path.
     */
    // TODO: QTBUG-125620 -- Refresh/reset import paths when loading a new component
    public <T extends QtQuickViewContent> void loadContent(T qmlContent, String[] qmlImportPaths)
            throws InvalidParameterException
    {
        String libName = qmlContent.getLibraryName();
        String qmlUri = qmlContent.getFilePath();

        if (libName == null || libName.isEmpty()) {
            throw new InvalidParameterException(
                    "QtQuickViewContent: return value of getLibraryName() may not be empty or null");
        }

        if (qmlUri == null || qmlUri.isEmpty()) {
            throw new InvalidParameterException(
                    "QtQuickViewContent: return value of getFilePath() may not be empty or null");
        }

        m_qmlUri = qmlUri;
        m_qmlImportPaths = qmlImportPaths;

        if (m_loadedComponent != null)
            m_loadedComponent.clear();

        m_loadedComponent = new WeakReference<>(qmlContent);
        qmlContent.detachView();
        qmlContent.attachView(this);
        // The first QQuickView creation happen after first libs loading
        // and windowReference() returns a reference to native QQuickView
        // instance, after that. We don't load library again if the view
        // exists.
        if (getWindowReference() == 0) {
            loadQtLibraries(libName);
        } else {
            createQuickView(m_qmlUri, getWidth(), getHeight(), 0, getWindowReference(),
                            m_qmlImportPaths);
        }
    }

    @Override
    void setWindowReference(long windowReference) {
        super.setWindowReference(windowReference);
        m_signalQueue.connectQueuedSignalListeners(this);
    }

    private boolean hasUnderlyingView() {
        return getWindowReference() != 0L;
    }

    /**
     * Loads QML content represented by a QtQuickViewContent. The library name and the qrc path of
     * the QML content will be extracted from the QtQuickViewContent to load the QML component.
     *
     * @param qmlContent an instance of a class that extends QtQuickViewContent
     * @throws InvalidParameterException if QtQuickViewContent does not contain valid information
     * about the module name, and the qrc path.
     */
    public <T extends QtQuickViewContent> void loadContent(T qmlContent)
            throws InvalidParameterException
    {
        loadContent(qmlContent, null);
    }

    @Override
    protected void createWindow(long parentWindowReference) {
        createQuickView(m_qmlUri, getWidth(), getHeight(), parentWindowReference, getWindowReference(),
                        m_qmlImportPaths);
    }

    /**
     * Sets the value of an existing property on the QML root object. The supported types are
     * {@link java.lang.Integer}, {@link java.lang.Double}, {@link java.lang.Float},
     * {@link java.lang.Boolean} and {@link java.lang.String}. These types get converted to their
     * corresponding QML types int, double/float, bool and string. This function does not add
     * properties to the QML root object if they do not exist, but prints a warning.
     *
     * @param propertyName the name of the existing root object property to set the value of
     * @param value        the value to set the property to QML's int, double/float, bool or
                           string
     * @see <a href="https://doc.qt.io/qt-6/qml-int.html">QML int</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-double.html">QML double/float</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-bool.html">QML bool</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-string.html">QML string</a>
     **/
    public void setProperty(String propertyName, Object value)
    {
        setRootObjectProperty(getWindowReference(), propertyName, value);
    }

    /**
     * Gets the value of an existing property of the QML root object. The supported types are
     * {@link java.lang.Integer}, {@link java.lang.Double}, {@link java.lang.Float},
     * {@link java.lang.Boolean} and {@link java.lang.String}. These types get converted to their
     * corresponding QML types int, double/float, bool and string. If the property does not
     * exist or the status of the QML component is anything other than
     * {@link QtQmlStatus#READY READY}, this function will return null.
     *
     * @param propertyName the name of the existing root object property
     * @throws ClassCastException if the returned type could not be casted to the requested type.
     * @see <a href="https://doc.qt.io/qt-6/qml-int.html">QML int</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-double.html">QML double/float</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-bool.html">QML bool</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-string.html">QML string</a>
     **/
    // getRootObjectProperty always returns a primitive type or an Object
    // so it is safe to suppress the unchecked warning
    @SuppressWarnings("unchecked")
    public <T> T getProperty(String propertyName)
    {
        return (T)getRootObjectProperty(getWindowReference(), propertyName);
    }

    /**
     * Connects a SignalListener to a signal of the QML root object. This will delay forming a
     * connection until the root object is ready.
     *
     * @param signalName the name of the root object signal
     * @param argType    the Class type of the signal argument
     * @param listener   an instance of the QtSignalListener interface
     * @return a connection id between signal and listener or the existing connection id if there is
     *         an existing connection between the same signal and listener. Return a negative value
     *         if the signal does not exists on the QML root object. Always returns
     *         <code>true</code> if the root object is not ready, as the connection is queued until
     *         the root object is ready.
     **/
    public <T> int connectSignalListener(String signalName, Class<T> argType,
                                         QtSignalListener<T> listener)
    {
        return connectSignalListener(signalName, new Class<?>[] { argType }, listener);
    }

    /**
     * Connects a SignalListener to a signal of the QML root object. Will delay forming a
     * connection until the root object is ready.
     *
     * @param signalName the name of the root object's signal
     * @param argTypes   the Class types of the signal arguments
     * @param listener   an instance of the {@link QtSignalListener} interface
     * @return a connection id between signal and listener or the existing connection id if there
     *         is an existing connection between the same signal and listener. Otherwise, a
     *         negative value is returned if the signal does not exist on the QML root object.
     *         Always returns <code>true</code> if the root object is not ready.
     **/
    public int connectSignalListener(String signalName, Class<?>[] argTypes, Object listener)
    {
        final int id = QtQuickViewContent.generateSignalId();
        connectSignalListener(signalName, argTypes, listener, id);
        return id;
    }

    /**
     * Convenience function to call the native addRootObjectSignalListener() method from
     * QtQuickViewContent.
     *
     * If the underlying C++ QAndroidQuickView has not been set yet, this connection is queued
     * until that happens. This is because the QAndroidQuickView does not exist yet, so we cannot
     * connect to its signals.
     *
     * This is provided as a way for QtSignalQueue to call addRootObjectSignalListener() while
     * providing a pre-generated ID. The regular public API generates its own ID, which is not
     * desired for queued signal connections.
     */
    void connectSignalListener(String signalName, Class<?>[] argTypes, Object listener, int id)
    {
        if (hasUnderlyingView())
            addRootObjectSignalListener(getWindowReference(), signalName, argTypes, listener, id);
        else
            m_signalQueue.add(signalName, argTypes, listener, id);
    }

    /**
     * Disconnects a SignalListener with a given id obtained from
     * {@link QtQuickView#connectSignalListener(String, Class, QtSignalListener)} or
     * {@link QtQuickView#connectSignalListener(String, Class[], Object)} call,
     * from listening to a signal.
     *
     * @param signalListenerId the connection id
     * @return Returns true if the connection id is valid and has been successfuly removed,
     *         otherwise returns false.
     **/
    public boolean disconnectSignalListener(int signalListenerId)
    {
        if (hasUnderlyingView())
            return removeRootObjectSignalListener(getWindowReference(), signalListenerId);
        else
            return m_signalQueue.remove(signalListenerId);
    }

    /**
     * Gets the status of the QML component.
     *
     * @return Returns QtQmlStatus.READY when the QML component is ready. Invoking methods that
     *         operate on the QML root object {@link QtQuickView#setProperty(String, Object)},
     *         {@link QtQuickView#getProperty(String)},
     *         {@link QtQuickView#connectSignalListener(String, Class, QtSignalListener)} and
     *         {@link QtQuickView#connectSignalListener(String, Class[], Object)} would
     *         succeed <b>only</b> if the current status is {@link QtQmlStatus#READY READY}.
     *         It can also return {@link QtQmlStatus#NULL NULL},
     *         {@link QtQmlStatus#LOADING LOADING}, or
     *         {@link QtQmlStatus#ERROR ERROR} based on the status of the underlying QQuickView
     *         instance.
     * @see <a href="https://doc.qt.io/qt-6/qquickview.html">QQuickView</a>
     **/
    public QtQmlStatus getStatus()
    {
        return m_lastStatus;
    }

    /**
     * Sets a QtQmlStatusChangeListener to listen to status changes.
     *
     * @param listener an instance of a QtQmlStatusChangeListener interface
     **/
    public void setStatusChangeListener(QtQmlStatusChangeListener listener)
    {
        m_statusChangeListener = listener;

        if (m_hasQueuedStatus) {
            sendStatusChanged(m_lastStatus);
            m_hasQueuedStatus = false;
        }
    }

    /**
     * Invokes a QML method of the root object.
     *
     * Supported parameter types are {@link java.lang.Integer},{@link java.lang.Double},
     * {@link java.lang.Float}, {@link java.lang.Boolean} and {@link java.lang.String}.
     * These types get converted to their corresponding types: <code>int</code>,
     * <code>double</code>, <code>real</code>, <code>bool</code>, and <code>string</code>,
     * respectively.
     *
     * @param methodName name of the method
     * @param params array of parameters that are passed to the method
     *
     * @see <a href="https://doc.qt.io/qt-6/qml-int.html">QML int</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-double.html">QML double</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-real.html">QML real</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-bool.html">QML bool</a>
     * @see <a href="https://doc.qt.io/qt-6/qml-string.html">QML string</a>
     **/
    public void invokeMethod(String methodName, Object[] params)
    {
        invokeMethod(getWindowReference(), methodName, params);
    }

    /**
     * Invokes a QML method of the root object.
     *
     * @param methodName name of the method
     *
     * @see QtQuickView#invokeMethod(String, Object[])
     */
    public void invokeMethod(String methodName)
    {
        invokeMethod(getWindowReference(), methodName, new Object[] {});
    }

    private void handleStatusChange(int status)
    {
        try {
            m_lastStatus = QtQmlStatus.fromInt(status);
        } catch (IllegalArgumentException e) {
            m_lastStatus = QtQmlStatus.NULL;
            e.printStackTrace();
        }

        if (m_statusChangeListener == null)
            m_hasQueuedStatus = true;
        else
            sendStatusChanged(m_lastStatus);
    }

    private void sendStatusChanged(QtQmlStatus status)
    {
        if (m_statusChangeListener != null) {
            QtQuickViewContent content = m_loadedComponent != null ? m_loadedComponent.get() : null;
            if (content == null)
                m_statusChangeListener.onStatusChanged(status);
            else
                m_statusChangeListener.onStatusChanged(status, content);
        }
    }
}
