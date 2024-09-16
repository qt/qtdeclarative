// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import java.lang.IllegalArgumentException;

/**
 * QtQmlStatus represents the QML component loading status.
 */
public enum QtQmlStatus {
    /**
     * Not loaded.
     **/
    NULL(0),

    /**
     * Loaded and ready.
     * Invoking methods that operate on a QML component would succeed <b>only<b> if
     * the current status is ready.
     **/
    READY(1),

    /**
     *The QML component is getting loaded from network.
     **/
    LOADING(2),

    /**
     * One or more errors has occurred during loading the QML component.
     **/
    ERROR(3);

    private final int m_value;

    QtQmlStatus(int value) { this.m_value = value; }

    QtQmlStatus() { this.m_value = ordinal(); }

    static QtQmlStatus fromInt(int value) throws IllegalArgumentException
    {
        for (QtQmlStatus enumValue : QtQmlStatus.values()) {
            if (enumValue.m_value == value) {
                return enumValue;
            }
        }
        throw new IllegalArgumentException("No QtQmlStatus enum with value " + value);
    }
}
