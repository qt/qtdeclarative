// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
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
 * </p>
 * <p>
 * <b>Known limitations:</b>
 * <p><ul>
 * <li> Only CMake is supported, not qmake.
 * <li> Only one QtQuickView can be added to your app, adding multiple outcomes unknown.
 * </ul><p>
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

    native void createQuickView(String qmlUri, int width, int height, long parentWindowReference,
                                long viewReference, String[] qmlImportPaths);
    native void setRootObjectProperty(long windowReference, String propertyName, Object value);
    native Object getRootObjectProperty(long windowReference, String propertyName);
    native int addRootObjectSignalListener(long windowReference, String signalName,
                                           Class<?> argType, Object listener);
    native boolean removeRootObjectSignalListener(long windowReference, int signalListenerId);

    /**
     * Creates a QtQuickView to load and view a QML component. Instantiating a QtQuickView will load
     * the Qt libraries, including the app library specified by <code>appName</code>. Then it
     * creates a QQuickView that loads the QML source specified by <code>qmlUri</code>.
     * <p>
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
     * <p>
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
     * <p>
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
        if (windowReference() == 0) {
            loadQtLibraries(libName);
        } else {
            createQuickView(m_qmlUri, getWidth(), getHeight(), 0, windowReference(),
                            m_qmlImportPaths);
        }
    }

    /**
     * Loads QML content represented by a QtQuickViewContent. The library name and the qrc path of
     * the QML content will be extracted from the QtQuickViewContent to load the QML component.
     * <p>
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
        createQuickView(m_qmlUri, getWidth(), getHeight(), parentWindowReference, windowReference(),
                        m_qmlImportPaths);
    }

    /**
     * Sets the value of an existing property on the QML root object. The supported types are
     * {@link java.lang.Integer}, {@link java.lang.Double}, {@link java.lang.Float},
     * {@link java.lang.Boolean} and {@link java.lang.String}. These types get converted to their
     * corresponding QML types int, double/float, bool and string. This function does not add
     * properties to the QML root object if they do not exist, but prints a warning.
     * <p>
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
        setRootObjectProperty(windowReference(), propertyName, value);
    }

    /**
     * Gets the value of an existing property of the QML root object. The supported types are
     * {@link java.lang.Integer}, {@link java.lang.Double}, {@link java.lang.Float},
     * {@link java.lang.Boolean} and {@link java.lang.String}. These types get converted to their
     * corresponding QML types int, double/float, bool and string. If the property does not
     * exist or the status of the QML component is anything other than
     * {@link QtQuickView#STATUS_READY STATUS_READY}, this function will return null.
     * <p>
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
        return (T)getRootObjectProperty(windowReference(), propertyName);
    }

    /**
     * Connects a SignalListener to a signal of the QML root object.
     * <p>
     * @param signalName the name of the root object signal
     * @param argType    the Class type of the signal argument
     * @param listener   an instance of the QtSignalListener interface
     * @return a connection id between signal and listener or the existing connection id if there is
     *         an existing connection between the same signal and listener. Return a negative value
     *         if the signal does not exists on the QML root object.
     **/
    public <T> int connectSignalListener(String signalName, Class<T> argType,
                                         QtSignalListener<T> listener)
    {
        int signalListenerId =
                addRootObjectSignalListener(windowReference(), signalName, argType, listener);
        if (signalListenerId < 0) {
            Log.w(TAG, "The signal " + signalName + " does not exist in the root object "
                                     + "or the arguments do not match with the listener.");
        }
        return signalListenerId;
    }

    /**
     * Disconnects a SignalListener with a given id obtained from
     * {@link QtQuickView#connectSignalListener() connectSignalListener} call, from listening to
     * a signal.
     * <p>
     * @param signalListenerId the connection id
     * @return Returns true if the connection id is valid and has been successfuly removed,
     *         otherwise returns false.
     **/
    public boolean disconnectSignalListener(int signalListenerId)
    {
        return removeRootObjectSignalListener(windowReference(), signalListenerId);
    }

    /**
     * Gets the status of the QML component.
     * <p>
     * @return Returns QtQmlStatus.READY when the QML component is ready. Invoking methods that
     *         operate on the QML root object ({@link QtQuickView#setProperty() setProperty},
     *         {@link QtQuickView#getProperty() getProperty}, and
     *         {@link QtQuickView#addSignalListener() addSignalListener}) would succeed <b>only</b>
     *         if the current status is QtQmlStatus.READY. It can also return QtQmlStatus.NULL,
     *         QtQmlStatus.LOADING, or QtQmlStatus.ERROR based on the status of the underlaying
               QQuickView instance.
     * @see <a href="https://doc.qt.io/qt-6/qquickview.html">QQuickView</a>
     **/
    public QtQmlStatus getStatus()
    {
        return m_lastStatus;
    }

    /**
     * Sets a QtQmlStatusChangeListener to listen to status changes.
     * <p>
     * @param listener an instance of a QtQmlStatusChangeListener interface
     **/
    public void setStatusChangeListener(QtQmlStatusChangeListener listener)
    {
        m_statusChangeListener = listener;

        if (m_hasQueuedStatus) {
            QtNative.runAction(() -> { m_statusChangeListener.onStatusChanged(m_lastStatus); });
            m_hasQueuedStatus = false;
        }
    }

    private void handleStatusChange(int status)
    {
        try {
            m_lastStatus = QtQmlStatus.fromInt(status);
        } catch (IllegalArgumentException e) {
            m_lastStatus = QtQmlStatus.NULL;
            e.printStackTrace();
        }

        if (m_statusChangeListener != null)
            QtNative.runAction(() -> {
                m_statusChangeListener.onStatusChanged(QtQmlStatus.fromInt(status));
            });
        else
            m_hasQueuedStatus = true;
    }
}
