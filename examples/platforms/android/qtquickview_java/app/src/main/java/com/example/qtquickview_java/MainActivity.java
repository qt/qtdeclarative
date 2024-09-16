// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qtquickview_java;

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
import org.qtproject.qt.android.QtQmlStatus;
import org.qtproject.qt.android.QtQmlStatusChangeListener;
import org.qtproject.qt.android.QtQuickView;
import org.qtproject.example.qtquickview.QmlModule.Main;
import org.qtproject.example.qtquickview.QmlModule.Second;
import java.util.HashMap;
import java.util.Map;

// Implement QtQuickView StatusChangeListener interface to get status updates
// from the underlying QQuickView
public class MainActivity extends AppCompatActivity implements QtQmlStatusChangeListener {

    private static final String TAG = "myTag";
    private final Colors m_colors = new Colors();
    private final Map<QtQmlStatus, String> m_statusNames = new HashMap<QtQmlStatus, String>()  {{
        put(QtQmlStatus.READY, " READY");
        put(QtQmlStatus.LOADING, " LOADING");
        put(QtQmlStatus.ERROR, " ERROR");
        put(QtQmlStatus.NULL, " NULL");
    }};
    private int m_qmlButtonSignalListenerId;
    private LinearLayout m_mainLinear;
    private FrameLayout m_qmlFrameLayout;
    private QtQuickView m_qtQuickView;
    //! [qmlContent]
    private final Main m_mainQmlContent = new Main();
    private final Second m_secondQmlContent = new Second();
    //! [qmlContent]
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
        m_qmlStatus = findViewById(R.id.qmlStatusText);
        m_androidControlsLayout = findViewById(R.id.javaLinear);
        m_box = findViewById(R.id.qmlColorBox);
        m_switch = findViewById(R.id.disconnectQmlListenerSwitch);
        m_switch.setOnClickListener(view -> switchListener());
        //! [m_qtQuickView]
        m_qtQuickView = new QtQuickView(this);
        //! [m_qtQuickView]

        // Set status change listener for m_qmlView
        // listener implemented below in OnStatusChanged
        //! [setStatusChangeListener]
        m_mainQmlContent.setStatusChangeListener(this);
        m_secondQmlContent.setStatusChangeListener(this);
        //! [setStatusChangeListener]
        //! [layoutParams]
        ViewGroup.LayoutParams params = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        m_qmlFrameLayout = findViewById(R.id.qmlFrame);
        m_qmlFrameLayout.addView(m_qtQuickView, params);
        //! [layoutParams]
        //! [loadContent]
        m_qtQuickView.loadContent(m_mainQmlContent);
        //! [loadContent]

        Button m_changeColorButton = findViewById(R.id.changeQmlColorButton);
        m_changeColorButton.setOnClickListener(view -> onClickListener());
        Button m_loadMainQmlButton = findViewById(R.id.loadMainQml);
        m_loadMainQmlButton.setOnClickListener(view -> loadMainQml());
        Button m_loadSecondQmlButton = findViewById(R.id.loadSecondQml);
        m_loadSecondQmlButton.setOnClickListener(view -> loadSecondQml());
        Button m_rotateQmlGridButton = findViewById(R.id.rotateQmlGridButton);
        m_rotateQmlGridButton.setOnClickListener(view -> rotateQmlGrid());

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

    //! [onClickListener]
    public void onClickListener() {
        // Set the QML view root object property "colorStringFormat" value to
        // color from Colors.getColor()
        m_mainQmlContent.setColorStringFormat(m_colors.getColor());

        String qmlBackgroundColor = m_mainQmlContent.getColorStringFormat();
        // Display the QML View background color code
        m_getPropertyValueText.setText(qmlBackgroundColor);

        // Display the QML View background color in a view
        // if qmlBackGroundColor is not null
        if (qmlBackgroundColor != null) {
            m_box.setBackgroundColor(Color.parseColor(qmlBackgroundColor));
        }
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
            m_mainQmlContent.disconnectSignalListener(m_qmlButtonSignalListenerId);
            //! [disconnect qml signal listener]
        } else {
            Log.i(TAG, "QML button onClicked signal listener connected");
            text.setText(R.string.disconnect_qml_button_signal_listener);
            m_qmlButtonSignalListenerId = m_mainQmlContent.connectOnClickedListener(
                    (String name, Void v) -> {
                        Log.i(TAG, "QML button clicked");
                        m_androidControlsLayout.setBackgroundColor(Color.parseColor(
                                m_colors.getColor()
                        ));
                    });
        }
    }

    //! [onStatusChanged]
    @Override
    public void onStatusChanged(QtQmlStatus qtQmlStatus) {
        Log.i(TAG, "Status of QtQuickView: " + qtQmlStatus);

        final String qmlStatus = getResources().getString(R.string.qml_view_status)
                + m_statusNames.get(qtQmlStatus);

        // Show current QML View status in a textview
        m_qmlStatus.setText(qmlStatus);

        // Connect signal listener to "onClicked" signal from main.qml
        // addSignalListener returns int which can be used later to identify the listener
        //! [qml signal listener]
        if (qtQmlStatus == QtQmlStatus.READY && !m_switch.isChecked()) {
            m_qmlButtonSignalListenerId = m_mainQmlContent.connectOnClickedListener(
                    (String name, Void v) -> {
                        Log.i(TAG, "QML button clicked");
                        m_androidControlsLayout.setBackgroundColor(Color.parseColor(
                                m_colors.getColor()
                        ));
                    });

        }
        //! [qml signal listener]
    }
    //! [onStatusChanged]
    //! [switchLoadedContent]
    private void loadSecondQml() {
        m_qtQuickView.loadContent(m_secondQmlContent);

        // Reset box color and color text after component reload
        m_box.setBackgroundColor(Color.parseColor("#00ffffff"));
        m_getPropertyValueText.setText("");
    }

    private void loadMainQml() {
        m_qtQuickView.loadContent(m_mainQmlContent);

        // Reset box color and color text after component reload
        m_box.setBackgroundColor(Color.parseColor("#00ffffff"));
        m_getPropertyValueText.setText("");
    }
    //! [switchLoadedContent]
    //! [gridRotate]
    private void rotateQmlGrid() {
        Integer previousGridRotation = m_secondQmlContent.getGridRotation();
        if (previousGridRotation != null) {
            m_secondQmlContent.setGridRotation(previousGridRotation + 45);
        }
    }
    //! [gridRotate]
}
