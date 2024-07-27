// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qtquickview_kotlin

import android.content.res.Configuration
import android.graphics.Color
import android.os.Bundle
import android.util.DisplayMetrics
import android.util.Log
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.LinearLayout
import androidx.appcompat.app.AppCompatActivity
import com.example.qtquickview_kotlin.databinding.ActivityMainBinding
import org.qtproject.example.qtquickview.QmlModule.Main
import org.qtproject.example.qtquickview.QmlModule.Second
import org.qtproject.qt.android.QtQmlStatus
import org.qtproject.qt.android.QtQmlStatusChangeListener
import org.qtproject.qt.android.QtQuickView


class MainActivity : AppCompatActivity(), QtQmlStatusChangeListener {

    private val TAG = "myTag"
    private val m_colors: Colors = Colors()
    private lateinit var m_binding: ActivityMainBinding
    private var m_qmlButtonSignalListenerId = 0
    private var m_qtQuickView: QtQuickView? = null
    //! [qmlComponent]
    private var m_mainQmlComponent: Main = Main()
    private val m_secondQmlComponent: Second = Second()
    //! [qmlComponent]
    private val m_statusNames = hashMapOf(
        QtQmlStatus.READY to "READY",
        QtQmlStatus.LOADING to "LOADING",
        QtQmlStatus.ERROR to "ERROR",
        QtQmlStatus.NULL to "NULL"
    )
    //! [onCreate]
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        //! [binding]
        m_binding = ActivityMainBinding.inflate(layoutInflater)
        val view = m_binding.root
        setContentView(view)
        //! [binding]

        m_binding.signalSwitch.setOnClickListener { switchListener() }

        //! [m_qtQuickView]
        m_qtQuickView = QtQuickView(this)
        //! [m_qtQuickView]

        // Set status change listener for m_qmlView
        // listener implemented below in OnStatusChanged
        //! [setStatusChangeListener]
        m_mainQmlComponent.setStatusChangeListener(this)
        m_secondQmlComponent.setStatusChangeListener(this)
        //! [setStatusChangeListener]

        //! [layoutParams]
        val params: ViewGroup.LayoutParams = FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT
        )
        m_binding.qmlFrame.addView(m_qtQuickView, params)
        //! [layoutParams]
        //! [loadComponent]
        m_qtQuickView!!.loadComponent(m_mainQmlComponent)
        //! [loadComponent]

        m_binding.changeColorButton.setOnClickListener { onClickListener() }
        m_binding.loadMainQml.setOnClickListener { loadMainQml() }
        m_binding.loadSecondQml.setOnClickListener { loadSecondQml() }
        m_binding.rotateQmlGridButton.setOnClickListener { rotateQmlGrid() }

        // Check target device orientation on launch
        handleOrientationChanges()
    }
    //! [onCreate]
    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        handleOrientationChanges()
    }

    private fun handleOrientationChanges() {
        // When specific target device display configurations (listed in AndroidManifest.xml
        // android:configChanges) change, get display metrics and make needed changes to UI
        val displayMetrics = DisplayMetrics()
        windowManager.defaultDisplay.getMetrics(displayMetrics)
        val qmlFrameLayoutParams = m_binding.qmlFrame.layoutParams
        val linearLayoutParams = m_binding.kotlinLinear.layoutParams

        if (displayMetrics.heightPixels > displayMetrics.widthPixels) {
            m_binding.mainLinear.orientation = LinearLayout.VERTICAL
            qmlFrameLayoutParams.width = ViewGroup.LayoutParams.MATCH_PARENT
            qmlFrameLayoutParams.height = 0
            linearLayoutParams.width = ViewGroup.LayoutParams.MATCH_PARENT
            linearLayoutParams.height = 0
        } else {
            m_binding.mainLinear.orientation = LinearLayout.HORIZONTAL
            qmlFrameLayoutParams.width = 0
            qmlFrameLayoutParams.height = ViewGroup.LayoutParams.MATCH_PARENT
            linearLayoutParams.width = 0
            linearLayoutParams.height = ViewGroup.LayoutParams.MATCH_PARENT
        }
        m_binding.qmlFrame.layoutParams = qmlFrameLayoutParams
        m_binding.kotlinLinear.layoutParams = linearLayoutParams
    }

    //! [onClickListener]
    private fun onClickListener() {
        // Set the QML view root object property "colorStringFormat" value to
        // color from Colors.getColor()
        m_mainQmlComponent.colorStringFormat = m_colors.getColor()

        val qmlBackgroundColor = m_mainQmlComponent.colorStringFormat

        // Display the QML View background color code
        m_binding.getPropertyValueText.text = qmlBackgroundColor

        // Display the QML View background color in a view
        // if qmlBackgroundColor is not null
        if (qmlBackgroundColor != null) {
            m_binding.colorBox.setBackgroundColor(Color.parseColor(qmlBackgroundColor))
        }
    }
    //! [onClickListener]

    private fun switchListener() {
        // Disconnect QML button signal listener if switch is On using the saved signal listener Id
        // and connect it again if switch is turned off
        if (m_binding.signalSwitch.isChecked) {
            Log.v(TAG, "QML button onClicked signal listener disconnected")
            m_binding.switchText.setText(R.string.connect_qml_button_signal_listener)
            //! [disconnect qml signal listener]
            m_mainQmlComponent.disconnectSignalListener(m_qmlButtonSignalListenerId)
            //! [disconnect qml signal listener]
        } else {
            Log.v(TAG, "QML button onClicked signal listener connected")
            m_binding.switchText.setText(R.string.disconnect_qml_button_signal_listener)
            m_qmlButtonSignalListenerId =
                m_mainQmlComponent.connectOnClickedListener { _: String, _: Void? ->
                    Log.i(TAG, "QML button clicked")
                    m_binding.kotlinLinear.setBackgroundColor(
                        Color.parseColor(
                            m_colors.getColor()
                        )
                    )
                }
        }
    }

    //! [onStatusChanged]
    override fun onStatusChanged(status: QtQmlStatus?) {
        Log.v(TAG, "Status of QtQuickView: $status")

        val qmlStatus = (resources.getString(R.string.qml_view_status)
                + m_statusNames[status])

        // Show current QML View status in a textview
        m_binding.qmlStatus.text = qmlStatus

        // Connect signal listener to "onClicked" signal from main.qml
        // addSignalListener returns int which can be used later to identify the listener
        //! [qml signal listener]
        if (status == QtQmlStatus.READY && !m_binding.signalSwitch.isChecked) {
            m_qmlButtonSignalListenerId =
                m_mainQmlComponent.connectOnClickedListener { _: String, _: Void? ->
                    Log.i(TAG, "QML button clicked")
                    m_binding.kotlinLinear.setBackgroundColor(
                        Color.parseColor(
                            m_colors.getColor()
                        )
                    )
                }
        }
        //! [qml signal listener]
    }
    //! [onStatusChanged]
    //! [switchLoadedComponent]
    private fun loadSecondQml() {
        m_qtQuickView!!.loadComponent(m_secondQmlComponent)

        // Reset box color and color text after component reload
        m_binding.colorBox.setBackgroundColor(Color.parseColor("#00ffffff"))
        m_binding.getPropertyValueText.text = ""
    }

    private fun loadMainQml() {
        m_qtQuickView!!.loadComponent(m_mainQmlComponent)

        // Reset box color and color text after component reload
        m_binding.colorBox.setBackgroundColor(Color.parseColor("#00ffffff"))
        m_binding.getPropertyValueText.text = ""
    }
    //! [switchLoadedComponent]
    //! [gridRotate]
    private fun rotateQmlGrid() {
        val previousGridRotation = m_secondQmlComponent.gridRotation
        if (previousGridRotation != null) {
            m_secondQmlComponent.gridRotation = previousGridRotation + 45
        }
    }
    //! [gridRotate]
}
