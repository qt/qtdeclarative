// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.qt.android.tst_qtquickview_basic;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import org.qtproject.qt.android.QtQmlStatus;
import org.qtproject.qt.android.QtQmlStatusChangeListener;
import org.qtproject.qt.android.QtQuickView;
import org.qtproject.qt.android.tst_qtquickview_basic_qml.TestViewModule.TestView;

public class TestActivity extends Activity implements QtQmlStatusChangeListener
{
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

        final FrameLayout qmlFrame = findViewById(R.id.qmlFrame);
        final ViewGroup.LayoutParams params = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        qmlFrame.addView(m_quickView, params);
        m_quickView.loadContent(m_testView);
    }

    @Override
    public void onStatusChanged(QtQmlStatus qtQmlStatus) {
        if (qtQmlStatus == QtQmlStatus.ERROR)
            finish(); // Error loading QML tests, just exit the app
    }

    public TestView testView() { return m_testView; }

    public static TestActivity instance() { return m_instance; }
}
