// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qtabstractlistmodel_kotlin

import java.util.UUID

import org.qtproject.qt.android.QtAbstractListModel
import org.qtproject.qt.android.QtModelIndex

class MyListModel : QtAbstractListModel() {
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

    private val m_dataList = ArrayList<String>()

    init {
        for (row in 0..4) {
            m_dataList.add(UUID.randomUUID().toString())
        }
    }

    override fun data(qtModelIndex: QtModelIndex, role: Int): Any {
        return when (DataRole.valueOf(role)) {
            DataRole.UUID -> "UUID: " + m_dataList[qtModelIndex.row()]
            DataRole.Row -> "Row: " + qtModelIndex.row()
            else -> ""
        }
    }

    override fun rowCount(qtModelIndex: QtModelIndex?): Int {
        return m_dataList.size
    }

    override fun roleNames(): HashMap<Int, String> {
        val m_roles = HashMap<Int, String>()
        m_roles[DataRole.UUID.value()] = "id"
        m_roles[DataRole.Row.value()] = "row"
        return m_roles
    }

    fun addRow() {
        beginInsertRows(QtModelIndex(), m_dataList.size, m_dataList.size)
        m_dataList.add(UUID.randomUUID().toString())
        endInsertRows()
    }

    fun removeRow() {
        if (!m_dataList.isEmpty()) {
            beginRemoveRows(QtModelIndex(), m_dataList.size - 1, m_dataList.size - 1)
            m_dataList.removeAt(m_dataList.size - 1)
            endRemoveRows()
        }
    }
}
