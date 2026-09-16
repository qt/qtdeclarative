// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qtquickview_kotlin

import android.graphics.Color
import android.os.Bundle
import android.util.Log
import android.view.ViewGroup
import android.widget.CompoundButton
import android.widget.FrameLayout
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import com.example.qtquickview_kotlin.databinding.ActivityMainBinding
//! [qmlTypeImports]
import org.qtproject.example.qtquickview.QmlModule.Main
import org.qtproject.example.qtquickview.QmlModule.Second
//! [qmlTypeImports]
import org.qtproject.qt.android.QtQmlStatus
import org.qtproject.qt.android.QtQmlStatusChangeListener
import org.qtproject.qt.android.QtQuickView
import org.qtproject.qt.android.QtQuickViewContent


class MainActivity : AppCompatActivity(), QtQmlStatusChangeListener {
    private val TAG = "myTag"
    private val m_colors: Colors = Colors()
    private lateinit var m_binding: ActivityMainBinding
    private var m_qmlButtonSignalListenerId = 0
    //! [qmlContent]
    private val m_firstQmlContent: Main = Main()
    private val m_secondQmlContent: Second = Second()
    //! [qmlContent]
    private val m_statusNames = hashMapOf(
        QtQmlStatus.READY to "READY",
        QtQmlStatus.LOADING to "LOADING",
        QtQmlStatus.ERROR to "ERROR",
        QtQmlStatus.NULL to "NULL"
    )
    //! [onCreate]
    override fun onCreate(savedInstanceState: Bundle?) {
        enableEdgeToEdge()
        super.onCreate(savedInstanceState)
        //! [binding]
        m_binding = ActivityMainBinding.inflate(layoutInflater)
        val view = m_binding.root
        setContentView(view)
        //! [binding]

        // Qt is just another view here, so insetting it is the host app's job.
        ViewCompat.setOnApplyWindowInsetsListener(m_binding.mainLinear) { view, windowInsets ->
            val insets = windowInsets.getInsets(
                WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout())
            view.setPadding(insets.left, insets.top, insets.right, insets.bottom)
            windowInsets
        }

        m_binding.disconnectQmlListenerSwitch.setOnCheckedChangeListener { button, checked ->
            switchListener(
                button,
                checked
            )
        }

        //! [m_qtQuickView]
        val firstQtQuickView = QtQuickView(this)
        val secondQtQuickView = QtQuickView(this)
        //! [m_qtQuickView]

        // Set status change listener for m_qmlView
        // listener implemented below in OnStatusChanged
        //! [setStatusChangeListener]
        m_firstQmlContent.setStatusChangeListener(this)
        m_secondQmlContent.setStatusChangeListener(this)
        //! [setStatusChangeListener]

        //! [layoutParams]
        val params: ViewGroup.LayoutParams = FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT
        )
        m_binding.firstQmlFrame.addView(firstQtQuickView, params)
        m_binding.secondQmlFrame.addView(secondQtQuickView, params)
        //! [layoutParams]
        //! [loadContent]
        firstQtQuickView.loadContent(m_firstQmlContent)
        secondQtQuickView.loadContent(m_secondQmlContent)
        //! [loadContent]

        m_binding.changeQmlColorButton.setOnClickListener { onClickListener() }
        m_binding.rotateQmlGridButton.setOnClickListener { rotateQmlGrid() }
    }
    //! [onCreate]

    //! [onClickListener]
    private fun onClickListener() {
        // Set the QML view root object property "colorStringFormat" value to
        // color from Colors.getColor()
        m_firstQmlContent.colorStringFormat = m_colors.getColor()
        updateColorDisplay()
    }

    private fun updateColorDisplay() {
        val qmlBackgroundColor = m_firstQmlContent.colorStringFormat
        // Display the QML View background color code
        m_binding.qmlViewBackgroundText.text = qmlBackgroundColor
        // Display the QML View background color in a view
        // if qmlBackgroundColor is not null
        if (qmlBackgroundColor != null) {
            m_binding.qmlColorBox.setBackgroundColor(Color.parseColor(qmlBackgroundColor))
        }
    }
    //! [onClickListener]

    private fun switchListener(buttonView: CompoundButton, isChecked: Boolean) {
        // Disconnect QML button signal listener if switch is On using the saved signal listener Id
        // and connect it again if switch is turned off
        if (isChecked) {
            m_qmlButtonSignalListenerId =
                m_firstQmlContent.connectOnClickedListener { _: String, _: Void? ->
                    m_binding.kotlinRelative.setBackgroundColor(
                        Color.parseColor(
                            m_colors.getColor()
                        )
                    )
                }
        } else {
            //! [disconnect qml signal listener]
            m_firstQmlContent.disconnectSignalListener(m_qmlButtonSignalListenerId)
            //! [disconnect qml signal listener]
        }
    }

    //! [onStatusChanged]
    override fun onStatusChanged(status: QtQmlStatus?, content: QtQuickViewContent?) {
        Log.v(TAG, "Status of QtQuickView: $status")

        // Show current QML View status in a textview
        m_binding.qmlStatusText.text = getString(R.string.qml_view_status, m_statusNames[status])

        updateColorDisplay()

        if (content == m_firstQmlContent) {
            // Connect signal listener to "onClicked" signal from main.qml
            // addSignalListener returns int which can be used later to identify the listener
            //! [qml signal listener]
            if (status == QtQmlStatus.READY && m_binding.disconnectQmlListenerSwitch.isChecked) {
                m_qmlButtonSignalListenerId =
                    m_firstQmlContent.connectOnClickedListener { _: String, _: Void? ->
                        Log.i(TAG, "QML button clicked")
                        m_binding.kotlinRelative.setBackgroundColor(
                            Color.parseColor(
                                m_colors.getColor()
                            )
                        )
                    }
            }
            //! [qml signal listener]
        }
    }
    //! [onStatusChanged]
    //! [gridRotate]
    private fun rotateQmlGrid() {
        val previousGridRotation = m_secondQmlContent.gridRotation
        if (previousGridRotation != null) {
            m_secondQmlContent.gridRotation = previousGridRotation + 45
        }
    }
    //! [gridRotate]
}
