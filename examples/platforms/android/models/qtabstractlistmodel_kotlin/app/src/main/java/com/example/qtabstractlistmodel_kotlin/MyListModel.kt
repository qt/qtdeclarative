// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qtabstractlistmodel_kotlin

import java.util.UUID

import org.qtproject.qt.android.QtAbstractListModel
import org.qtproject.qt.android.QtModelIndex

//! [MyListModel definition]
class MyListModel : QtAbstractListModel() {
    private val m_dataList = ArrayList<String>()

    init {
        synchronized(this) {
            for (row in 0..4) {
                m_dataList.add(UUID.randomUUID().toString())
            }
        }
    }
//! [MyListModel definition]

//! [MyListModel::data]
    @Synchronized
    override fun data(qtModelIndex: QtModelIndex, role: Int): Any {
        return when (DataRole.valueOf(role)) {
            DataRole.UUID -> "UUID: " + m_dataList[qtModelIndex.row()]
            DataRole.Row -> "Row: " + qtModelIndex.row()
            else -> ""
        }
    }
//! [MyListModel::data]

    @Synchronized
    override fun rowCount(qtModelIndex: QtModelIndex?): Int {
        return m_dataList.size
    }

//! [MyListModel::roleNames]
    @Synchronized
    override fun roleNames(): HashMap<Int, String> {
        val m_roles = HashMap<Int, String>()
        m_roles[DataRole.UUID.value()] = "id"
        m_roles[DataRole.Row.value()] = "row"
        return m_roles
    }
//! [MyListModel::roleNames]

//! [MyListModel::addRow]
    @Synchronized
    fun addRow() {
        beginInsertRows(QtModelIndex(), m_dataList.size, m_dataList.size)
        m_dataList.add(UUID.randomUUID().toString())
        endInsertRows()
    }
//! [MyListModel::addRow]

//! [MyListModel::removeRow]
    @Synchronized
    fun removeRow() {
        if (!m_dataList.isEmpty()) {
            beginRemoveRows(QtModelIndex(), m_dataList.size - 1, m_dataList.size - 1)
            m_dataList.removeAt(m_dataList.size - 1)
            endRemoveRows()
        }
    }
//! [MyListModel::removeRow]

//! [MyListModel::DataRole enum]
    private enum class DataRole(val m_value: Int) {
        UUID(0),
        Row(1);

        fun value(): Int {
            return m_value
        }

        companion object {
            fun valueOf(value: Int): DataRole? {
                val values = entries.toTypedArray()
                if (0 <= value && value < values.size) return values[value]
                return null
            }
        }
    }
//! [MyListModel::DataRole enum]
}
