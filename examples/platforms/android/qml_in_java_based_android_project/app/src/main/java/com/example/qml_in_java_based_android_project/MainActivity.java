// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qml_in_java_based_android_project;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SwitchCompat;
import android.content.res.Configuration;
import android.graphics.Color;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.qtproject.qt.android.QtQuickView;

import java.util.HashMap;
import java.util.Map;


// Implement QtQuickView StatusChangeListener interface to get status updates
// from the underlying QQuickView
public class MainActivity extends AppCompatActivity implements QtQuickView.StatusChangeListener {

    private static final String TAG = "myTag";
    private final Colors m_colors = new Colors();
    private final Map<Integer, String> m_statusNames = new HashMap<Integer, String>()  {{
        put(QtQuickView.STATUS_READY, " READY");
        put(QtQuickView.STATUS_LOADING, " LOADING");
        put(QtQuickView.STATUS_ERROR, " ERROR");
        put(QtQuickView.STATUS_NULL, " NULL");
    }};
    private int m_qmlButtonSignalListenerId;
    private LinearLayout m_mainLinear;
    private FrameLayout m_qmlFrameLayout;
    private QtQuickView m_qmlView;
    private LinearLayout m_androidControlsLayout;
    private TextView m_getPropertyValueText;
    private TextView m_qmlStatus;
    private SwitchCompat m_switch;
    private View m_box;

    //! [onCreate]
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        m_mainLinear = findViewById(R.id.mainLinear);
        m_getPropertyValueText = findViewById(R.id.getPropertyValueText);
        m_qmlStatus = findViewById(R.id.qmlStatus);
        m_androidControlsLayout = findViewById(R.id.javaLinear);
        m_box = findViewById(R.id.box);

        m_switch = findViewById(R.id.switch1);
        m_switch.setOnClickListener(view -> switchListener());
        //! [m_qmlView]
        m_qmlView = new QtQuickView(this, "qrc:/qt/qml/qml_in_android_view/main.qml",
                "qml_in_android_view");
        //! [m_qmlView]

        // Set status change listener for m_qmlView
        // listener implemented below in OnStatusChanged
        //! [setStatusChangeListener]
        m_qmlView.setStatusChangeListener(this);
        //! [setStatusChangeListener]
        //! [layoutParams]
        ViewGroup.LayoutParams params = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        m_qmlFrameLayout = findViewById(R.id.qmlFrame);
        m_qmlFrameLayout.addView(m_qmlView, params);
        //! [layoutParams]
        Button button = findViewById(R.id.button);
        button.setOnClickListener(view -> onClickListener());

        // Check target device orientation on launch
        handleOrientationChanges();
    }
    //! [onCreate]
    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        handleOrientationChanges();
    }

    private void handleOrientationChanges() {
        // When specific target device display configurations (listed in AndroidManifest.xml
        // android:configChanges) change, get display metrics and make needed changes to UI
        DisplayMetrics displayMetrics = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        ViewGroup.LayoutParams qmlFrameLayoutParams = m_qmlFrameLayout.getLayoutParams();
        ViewGroup.LayoutParams linearLayoutParams = m_androidControlsLayout.getLayoutParams();

        if (displayMetrics.heightPixels > displayMetrics.widthPixels) {
            m_mainLinear.setOrientation(LinearLayout.VERTICAL);
            qmlFrameLayoutParams.width = ViewGroup.LayoutParams.MATCH_PARENT;
            qmlFrameLayoutParams.height = 0;
            linearLayoutParams.width = ViewGroup.LayoutParams.MATCH_PARENT;
            linearLayoutParams.height = 0;
        } else {
            m_mainLinear.setOrientation(LinearLayout.HORIZONTAL);
            qmlFrameLayoutParams.width = 0;
            qmlFrameLayoutParams.height = ViewGroup.LayoutParams.MATCH_PARENT;
            linearLayoutParams.width = 0;
            linearLayoutParams.height = ViewGroup.LayoutParams.MATCH_PARENT;
        }
        m_qmlFrameLayout.setLayoutParams(qmlFrameLayoutParams);
        m_androidControlsLayout.setLayoutParams(linearLayoutParams);
    }

    //! [onStatusChanged]
    @Override
    public void onStatusChanged(int status) {
        Log.i(TAG, "Status of QtQuickView: " + status);

        final String qmlStatus = getResources().getString(R.string.qml_view_status)
                + m_statusNames.get(status);

        // Show current QML View status in a textview
        m_qmlStatus.setText(qmlStatus);

        // Connect signal listener to "onClicked" signal from main.qml
        // addSignalListener returns int which can be used later to identify the listener
        //! [qml signal listener]
        if (status == QtQuickView.STATUS_READY && !m_switch.isChecked()) {
            m_qmlButtonSignalListenerId = m_qmlView.connectSignalListener("onClicked", Object.class,
                    (String signal, Object o) -> {
                Log.i(TAG, "QML button clicked");
                m_androidControlsLayout.setBackgroundColor(Color.parseColor(m_colors.getColor()));
            });

        }
        //! [qml signal listener]
    }
    //! [onStatusChanged]
    //! [onClickListener]
    public void onClickListener() {
        // Set the QML view root object property "colorStringFormat" value to
        // color from Colors.getColor()
        m_qmlView.setProperty("colorStringFormat", m_colors.getColor());

        String qmlBackgroundColor = m_qmlView.getProperty("colorStringFormat");

        // Display the QML View background color code
        m_getPropertyValueText.setText(qmlBackgroundColor);

        // Display the QML View background color in a view
        m_box.setBackgroundColor(Color.parseColor(qmlBackgroundColor));
    }
    //! [onClickListener]

    public void switchListener() {
        TextView text = findViewById(R.id.switchText);
        // Disconnect QML button signal listener if switch is On using the saved signal listener Id
        // and connect it again if switch is turned off
        if (m_switch.isChecked()) {
            Log.i(TAG, "QML button onClicked signal listener disconnected");
            text.setText(R.string.connect_qml_button_signal_listener);
            //! [disconnect qml signal listener]
            m_qmlView.disconnectSignalListener(m_qmlButtonSignalListenerId);
            //! [disconnect qml signal listener]
        } else {
            Log.i(TAG, "QML button onClicked signal listener connected");
            text.setText(R.string.disconnect_qml_button_signal_listener);
            m_qmlButtonSignalListenerId = m_qmlView.connectSignalListener("onClicked",
                    Object.class, (String t, Object value) -> {
                Log.i(TAG, "QML button clicked");
                m_androidControlsLayout.setBackgroundColor(Color.parseColor(m_colors.getColor()));
            });
        }
    }
}

