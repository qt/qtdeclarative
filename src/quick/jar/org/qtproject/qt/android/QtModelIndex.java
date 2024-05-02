// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

public class QtModelIndex
{
    public QtModelIndex() { }
    public int column() { return (int)m_privateData[1]; }
    public native Object data(int role);
    public native long internalId();
    public native boolean isValid();
    public native QtModelIndex parent();
    public int row() { return (int)m_privateData[0]; }

    private long[] m_privateData = { -1 /*row*/, -1 /*column*/, 0 /*internalId*/,
                                     0 /*modelReference*/ };
    private QtModelIndex m_parent = null;
    private QtModelIndex(int row, int column, long internalId, long modelReference)
    {
        m_privateData[0] = row;
        m_privateData[1] = column;
        m_privateData[2] = internalId;
        m_privateData[3] = modelReference;
        m_parent = null;
    }
    private QtModelIndex(int row, int column, QtModelIndex parent, long modelReference)
    {
        m_privateData[0] = row;
        m_privateData[1] = column;
        m_privateData[2] = 0;
        m_privateData[3] = modelReference;
        m_parent = parent;
    }
    private void detachFromNative()
    {
        m_privateData[0] = -1;
        m_privateData[1] = -1;
        m_privateData[2] = 0;
        m_privateData[3] = 0;
    };
}
