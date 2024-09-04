// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package com.example.qtquickview_java;

import android.annotation.SuppressLint;
import android.app.Service;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.os.IBinder;
import android.util.Size;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.Gravity;

import android.widget.Button;
import android.widget.Switch;
import android.widget.TextView;

import java.util.Random;
import java.util.function.Consumer;

import org.qtproject.qt.android.QtQuickView;
import org.qtproject.qt.android.QtQmlStatus;
import org.qtproject.qt.android.QtQmlStatusChangeListener;
import org.qtproject.example.qtquickview_service.Qml_floating_view.Main;

@SuppressLint("UseSwitchCompatOrMaterialCode")
public class QmlService extends Service implements QtQmlStatusChangeListener
{
    private static final String TAG = "QmlService";
    private WindowManager m_windowManager;
    private QtQuickView m_serviceView;
    private final Main m_serviceViewContent = new Main();
    private View m_mainView;

    private TextView m_qmlBackgroundColorTextView;
    private TextView m_qmlStatusTextView;
    private View m_colorBox;
    private Switch m_connectionSwitch;
    private int m_qmlSignalListenerId;

    @Override
    public IBinder onBind(Intent intent)
    {
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public void onCreate()
    {
        m_windowManager = getSystemService(WindowManager.class);

        getScreenSize((size) -> {
            // Get the available geometry, and split it between the Android and QML UIs
            m_serviceView = addQuickView(new Size(size.getWidth() / 2, size.getHeight()));
            m_serviceViewContent.setStatusChangeListener(this);
            m_serviceView.loadContent(m_serviceViewContent);

            m_mainView = addMainView(new Size(size.getWidth() / 2, size.getHeight()));
            connectToNativeControls(m_mainView);
        });
    }

    /*
        Draw the "main" view on the left side of the screen, with the native controls
     */
    private View addMainView(final Size size)
    {
        final LayoutInflater inflater = getSystemService(LayoutInflater.class);

        final WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams(
                size.getWidth(), size.getHeight(),
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
                        | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                        & ~WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH,
                PixelFormat.TRANSLUCENT);
        layoutParams.gravity = Gravity.LEFT | Gravity.CENTER_VERTICAL;

        View mainView = inflater.inflate(R.layout.view_main, null);
        m_windowManager.addView(mainView, layoutParams);
        return mainView;
    }

    /*
        Take size, and draw QtQuickView of that size on the right side of the screen
     */
    private QtQuickView addQuickView(final Size size)
    {
        WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams(
                size.getWidth(), size.getHeight(),
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
                        | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                        & ~WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH,
                PixelFormat.TRANSLUCENT);
        layoutParams.gravity = Gravity.RIGHT | Gravity.CENTER_VERTICAL;

        QtQuickView serviceView = new QtQuickView(this);
        m_windowManager.addView(serviceView, layoutParams);
        return serviceView;
    }

    /*
        Draw empty View that fills the parent (screen in this case) to discover the available size,
        report to consumer
     */
    private void getScreenSize(final Consumer<Size> screenSizeConsumer)
    {
        final WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
                        | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                                & ~WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH,
                PixelFormat.TRANSLUCENT);
        final View view = new View(this) {
            @Override
            protected void onLayout(boolean changed, int left, int top, int right, int bottom)
            {
                m_windowManager.removeView(this);
                screenSizeConsumer.accept(new Size(right - left, bottom - top));
            }
        };
        m_windowManager.addView(view, params);
    }

    @Override
    public void onDestroy()
    {
        if (m_windowManager != null) {
            if (m_serviceView != null) {
                m_windowManager.removeView(m_serviceView);
                m_serviceView = null;
            }
            if (m_mainView != null) {
                m_windowManager.removeView(m_mainView);
                m_mainView = null;
            }
        }
    }

    /*
        Connect listeners to the native controls
     */
    private void connectToNativeControls(final View mainView)
    {
        m_qmlBackgroundColorTextView = mainView.findViewById(R.id.qmlBackgroundColorText);
        m_qmlStatusTextView = mainView.findViewById(R.id.qmlStatus);
        m_colorBox = mainView.findViewById(R.id.box);

        m_connectionSwitch = mainView.findViewById(R.id.switch1);
        m_connectionSwitch.setOnCheckedChangeListener(
                (buttonView, isChecked) -> connectSwitchListener(isChecked));

        final Button changeColorButton = mainView.findViewById(R.id.button);
        changeColorButton.setOnClickListener(this::onChangeColorButtonListener);
    }

    public void onChangeColorButtonListener(View view)
    {
        m_serviceViewContent.setColorStringFormat(getRandomColorString());

        final String qmlColor = m_serviceView.getProperty("colorStringFormat");
        m_qmlBackgroundColorTextView.setText(qmlColor);
        m_colorBox.setBackgroundColor(Color.parseColor(qmlColor));
    }

    @Override
    public void onStatusChanged(QtQmlStatus status)
    {
        m_qmlStatusTextView.setText(
                String.format("%s %s", getResources().getString(R.string.qml_view_status), status));
        // Once QML is loaded and the signal listener switch is not checked,
        // connect to onClicked() signal in main.qml
        if (status == QtQmlStatus.READY && m_connectionSwitch.isChecked())
            connectSwitchListener(m_connectionSwitch.isChecked());
    }

    private void connectSwitchListener(boolean checked)
    {
        if (checked) {
            m_qmlSignalListenerId = m_serviceView.connectSignalListener(
                    "onClicked", Object.class, this::onQmlChangeColorButtonClicked);
        } else {
            m_serviceView.disconnectSignalListener(m_qmlSignalListenerId);
        }
    }

    public void onQmlChangeColorButtonClicked(String signal, Object o)
    {
        m_mainView.setBackgroundColor(Color.parseColor(getRandomColorString()));
    }

    private String getRandomColorString()
    {
        Random rand = new Random();
        return String.format("#%06x", rand.nextInt(0xffffff + 1));
    }
}
