// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qtabstractlistmodel_kotlin

import android.os.Bundle
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity

import org.qtproject.example.qtabstractlistmodel.QmlModule.Main
import org.qtproject.qt.android.QtQmlStatus
import org.qtproject.qt.android.QtQmlStatusChangeListener
import org.qtproject.qt.android.QtQuickView

//! [MainActivity definition]
class MainActivity : AppCompatActivity(), QtQmlStatusChangeListener {
    private val m_mainQmlContent: Main = Main()
    private val m_listModel = MyListModel()
//! [MainActivity definition]

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
//! [Adding QtQuickView]
        val qtQuickView: QtQuickView = QtQuickView(this)

        val params: ViewGroup.LayoutParams = FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT
        )
        val qmlFrameLayout: FrameLayout = findViewById<FrameLayout>(R.id.qmlFrame)
        qmlFrameLayout.addView(qtQuickView, params)
//! [Adding QtQuickView]
//! [Adding control buttons]
        val addRowAtEndButton: Button = findViewById<Button>(R.id.addRow)
        val removeRowFromEndButton: Button = findViewById<Button>(R.id.removeRow)
        addRowAtEndButton.setOnClickListener { _: View? ->
            m_listModel.addRow()
        }
        removeRowFromEndButton.setOnClickListener { _: View? ->
            m_listModel.removeRow()
        }
//! [Adding control buttons]
//! [Loading the QML component]
        m_mainQmlContent.setStatusChangeListener(this)
        qtQuickView.loadContent(m_mainQmlContent)
//! [Loading the QML component]
    }

//! [Linking the data model]
    override fun onStatusChanged(qtQmlStatus: QtQmlStatus) {
        if (qtQmlStatus === QtQmlStatus.READY) {
            m_mainQmlContent.setDataModel(m_listModel)
        }
    }
//! [Linking the data model]
}
