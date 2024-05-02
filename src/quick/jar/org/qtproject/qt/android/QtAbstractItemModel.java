// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import java.util.HashMap;

public abstract class QtAbstractItemModel
{
    public QtAbstractItemModel(){};
    public abstract int columnCount(QtModelIndex parent);
    public abstract Object data(QtModelIndex index, int role);
    public abstract QtModelIndex index(int row, int column, QtModelIndex parent);
    public abstract QtModelIndex parent(QtModelIndex index);
    public abstract int rowCount(QtModelIndex parent);

    public native boolean canFetchMore(QtModelIndex parent);
    public native void fetchMore(QtModelIndex parent);
    public native boolean hasChildren(QtModelIndex parent);
    public native boolean hasIndex(int row, int column, QtModelIndex parent);

    public HashMap<Integer, String> roleNames()
    {
        return (HashMap<Integer, String>)jni_roleNames();
    }

    protected final void beginInsertColumns(QtModelIndex parent, int first, int last)
    {
        jni_beginInsertColumns(parent, first, last);
    }

    protected final void beginInsertRows(QtModelIndex parent, int first, int last)
    {
        jni_beginInsertRows(parent, first, last);
    }
    protected final boolean beginMoveColumns(QtModelIndex sourceParent, int sourceFirst,
                                             int sourceLast, QtModelIndex destinationParent,
                                             int destinationChild)
    {
        return jni_beginMoveColumns(sourceParent, sourceFirst, sourceLast, destinationParent,
                                    destinationChild);
    }
    protected final boolean beginMoveRows(QtModelIndex sourceParent, int sourceFirst,
                                          int sourceLast, QtModelIndex destinationParent,
                                          int destinationChild)
    {
        return jni_beginMoveRows(sourceParent, sourceFirst, sourceLast, destinationParent,
                                 destinationChild);
    }
    protected final void beginRemoveColumns(QtModelIndex parent, int first, int last)
    {
        jni_beginRemoveColumns(parent, first, last);
    }
    protected final void beginRemoveRows(QtModelIndex parent, int first, int last)
    {
        jni_beginRemoveRows(parent, first, last);
    }
    protected final void beginResetModel() { jni_beginResetModel(); }

    protected final QtModelIndex createIndex(int row, int column, long id)
    {
        return (QtModelIndex)jni_createIndex(row, column, id);
    }
    protected final void endInsertColumns() { jni_endInsertColumns(); }
    protected final void endInsertRows() { jni_endInsertRows(); }
    protected final void endMoveColumns() { jni_endMoveColumns(); }
    protected final void endMoveRows() { jni_endMoveRows(); }
    protected final void endRemoveColumns() { jni_endRemoveColumns(); }
    protected final void endRemoveRows() { jni_endRemoveRows(); }
    protected final void endResetModel() { jni_endResetModel(); }

    private native void jni_beginInsertColumns(QtModelIndex parent, int first, int last);
    private native void jni_beginInsertRows(QtModelIndex parent, int first, int last);
    private native boolean jni_beginMoveColumns(QtModelIndex sourceParent, int sourceFirst,
                                                int sourceLast, QtModelIndex destinationParent,
                                                int destinationChild);
    private native boolean jni_beginMoveRows(QtModelIndex sourceParent, int sourceFirst,
                                             int sourceLast, QtModelIndex destinationParent,
                                             int destinationChild);
    private native void jni_beginRemoveColumns(QtModelIndex parent, int first, int last);
    private native void jni_beginRemoveRows(QtModelIndex parent, int first, int last);
    private native void jni_beginResetModel();
    private native Object jni_createIndex(int row, int column, long id);
    private native void jni_endInsertColumns();
    private native void jni_endInsertRows();
    private native void jni_endMoveColumns();
    private native void jni_endMoveRows();
    private native void jni_endRemoveColumns();
    private native void jni_endRemoveRows();
    private native void jni_endResetModel();
    private native Object jni_roleNames();

    private long m_nativeReference = 0;
    private QtAbstractItemModel(long nativeReference) { m_nativeReference = nativeReference; }
    private void detachFromNative() { m_nativeReference = 0; };
    private long nativeReference() { return m_nativeReference; }
    private void setNativeReference(long nativeReference) { m_nativeReference = nativeReference; }
    private static boolean instanceOf(Object obj) { return (obj instanceof QtAbstractItemModel); }
}
