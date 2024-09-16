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
import org.qtproject.example.qtquickview_service.Qml_floating_view.Main;
import org.qtproject.example.qtquickview_service.Qml_floating_view.Second;

@SuppressLint("UseSwitchCompatOrMaterialCode")
public class QmlService extends Service
{
    private static final String TAG = "QmlService";
    private WindowManager m_windowManager;

    private final Main m_serviceViewContent = new Main();
    private final Second m_secondServiceViewContent = new Second();

    private QtQuickView m_serviceView;
    private QtQuickView m_serviceView2;

    private int m_qmlSignalListenerId;
    private int m_qmlSignalListenerId2;

    private View m_mainView;

    private TextView m_qmlBackgroundColorTextView;
    private TextView m_qmlBackgroundColorTextView2;
    private TextView m_qmlStatusTextView;
    private TextView m_qmlStatusTextView2;
    private View m_colorBox;
    private View m_colorBox2;

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
            final Size qmlViewSize = new Size(size.getWidth() / 2, size.getHeight() / 2);

            WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams(
                    qmlViewSize.getWidth(), qmlViewSize.getHeight(),
                    WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                    WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
                            | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                                    & ~WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH,
                    PixelFormat.TRANSLUCENT);

            // Get the available geometry, and split it between the Android and QML UIs
            layoutParams.gravity = Gravity.END | Gravity.TOP;
            m_serviceView = addQuickView(qmlViewSize, layoutParams);
            m_serviceViewContent.setStatusChangeListener((qtQmlStatus -> {
                if (qtQmlStatus == QtQmlStatus.READY)
                    m_qmlSignalListenerId = m_serviceViewContent.connectOnClickedListener(
                            this::onQmlChangeColorButtonClicked);
                m_qmlStatusTextView.setText(String.format(
                        "%s %s", getResources().getString(R.string.qml_view_status), qtQmlStatus));
            }));
            m_serviceView.loadContent(m_serviceViewContent);

            layoutParams.gravity = Gravity.END | Gravity.BOTTOM;
            m_serviceView2 = addQuickView(qmlViewSize, layoutParams);
            m_secondServiceViewContent.setStatusChangeListener((qtQmlStatus -> {
                if (qtQmlStatus == QtQmlStatus.READY)
                    m_qmlSignalListenerId2 = m_secondServiceViewContent.connectOnClickedListener(
                            this::onQmlChangeColorButtonClicked);
                m_qmlStatusTextView2.setText(
                        String.format("%s %s", getResources().getString(R.string.qml_view_status_2),
                                      qtQmlStatus));
            }));
            m_serviceView2.loadContent(m_secondServiceViewContent);

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
        layoutParams.gravity = Gravity.START | Gravity.CENTER_VERTICAL;

        View mainView = inflater.inflate(R.layout.view_main, null);
        m_windowManager.addView(mainView, layoutParams);
        return mainView;
    }

    /*
        Take size, and draw QtQuickView of that size on the right side of the screen
     */
    private QtQuickView addQuickView(final Size size, WindowManager.LayoutParams layoutParams)
    {
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
            if (m_serviceView2 != null) {
                m_windowManager.removeView(m_serviceView2);
                m_serviceView2 = null;
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
        // View 1
        m_qmlBackgroundColorTextView = mainView.findViewById(R.id.qmlBackgroundColorText);
        m_qmlStatusTextView = mainView.findViewById(R.id.qmlStatus);
        m_colorBox = mainView.findViewById(R.id.box);

        final Switch connectionSwitch = mainView.findViewById(R.id.switch1);
        connectionSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                m_qmlSignalListenerId = m_serviceViewContent.connectOnClickedListener(
                        this::onQmlChangeColorButtonClicked);
            } else {
                m_serviceViewContent.disconnectSignalListener(m_qmlSignalListenerId);
            }
        });

        final Button changeColorButton = mainView.findViewById(R.id.button);
        changeColorButton.setOnClickListener((view) -> {
            m_serviceViewContent.setColorStringFormat(getRandomColorString());
            final String qmlColor = m_serviceView.getProperty("colorStringFormat");
            m_qmlBackgroundColorTextView.setText(qmlColor);
            m_colorBox.setBackgroundColor(Color.parseColor(qmlColor));
        });

        // View 2
        m_qmlBackgroundColorTextView2 = mainView.findViewById(R.id.qmlBackgroundColorText2);
        m_qmlStatusTextView2 = mainView.findViewById(R.id.qmlStatus2);
        m_colorBox2 = mainView.findViewById(R.id.box2);

        final Switch connectionSwitch2 = mainView.findViewById(R.id.switch2);
        connectionSwitch2.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                m_qmlSignalListenerId2 = m_secondServiceViewContent.connectOnClickedListener(
                        this::onQmlChangeColorButtonClicked);
            } else {
                m_secondServiceViewContent.disconnectSignalListener(m_qmlSignalListenerId2);
            }
        });

        final Button changeColorButton2 = mainView.findViewById(R.id.button2);
        changeColorButton2.setOnClickListener((view) -> {
            m_secondServiceViewContent.setColorStringFormat(getRandomColorString());
            final String qmlColor = m_serviceView2.getProperty("colorStringFormat");
            m_qmlBackgroundColorTextView2.setText(qmlColor);
            m_colorBox2.setBackgroundColor(Color.parseColor(qmlColor));
        });
    }

    public void onQmlChangeColorButtonClicked(String signal, Void unused)
    {
        m_mainView.setBackgroundColor(Color.parseColor(getRandomColorString()));
    }

    private String getRandomColorString()
    {
        Random rand = new Random();
        return String.format("#%06x", rand.nextInt(0xffffff + 1));
    }
}
