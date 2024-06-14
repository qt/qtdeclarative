// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

/**
 * A callback that notifies clients about the status of QML component loading.
 **/
public interface QtQmlStatusChangeListener
{
    /**
     * Called on the Android UI thread when the QML component status has changed.
     * @param status The current status. The status can be QtQmlStatus.NULL,
     *               QtQmlStatus.READY, QtQmlStatus.LOADING, or QtQmlStatus.ERROR.
     **/
    void onStatusChanged(QtQmlStatus status);
}
