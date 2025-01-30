// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.qt.android.qtquickview_signallistener;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import org.qtproject.qt.android.QtQmlStatus;
import org.qtproject.qt.android.QtQmlStatusChangeListener;
import org.qtproject.qt.android.QtQuickView;
import org.qtproject.qt.android.qtquickview_signallistener_qml.tst_qtquickview_signallistener_qml.TestViewModule.TestView;

public class TestActivity extends Activity implements QtQmlStatusChangeListener
{
    native void onBasicSignal();
    native void onIntSignal(Integer value);
    native void onBoolSignal(Boolean value);
    native void onDoubleSignal(Double value);
    native void onStringSignal(String value);

    private final TestView m_testView = new TestView();
    private QtQuickView m_quickView;
    private static TestActivity m_instance;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        m_instance = this;
        m_quickView = new QtQuickView(this);
        m_testView.setStatusChangeListener(this);

        final FrameLayout qmlFrame = findViewById(R.id.qmlFrame);
        final ViewGroup.LayoutParams params = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        qmlFrame.addView(m_quickView, params);
        m_quickView.loadContent(m_testView);
    }

    @Override public void onStatusChanged(QtQmlStatus qtQmlStatus)
    {
        if (qtQmlStatus == QtQmlStatus.READY) {
            m_testView.connectBasicSignalListener((String unused, Void _unused) -> onBasicSignal());
            m_testView.connectIntSignalListener(
                    (String unused, Integer value) -> onIntSignal(value));
            m_testView.connectBoolSignalListener(
                    (String unused, Boolean value) -> onBoolSignal(value));
            m_testView.connectDoubleSignalListener(
                    (String unused, Double value) -> onDoubleSignal(value));
            m_testView.connectStringSignalListener(
                    (String unused, String value) -> onStringSignal(value));
        } else if (qtQmlStatus == QtQmlStatus.ERROR) {
            finish(); // Error loading QML tests, just exit the app
        }
    }

    public TestView testView() { return m_testView; }

    public static TestActivity instance() { return m_instance; }
}
