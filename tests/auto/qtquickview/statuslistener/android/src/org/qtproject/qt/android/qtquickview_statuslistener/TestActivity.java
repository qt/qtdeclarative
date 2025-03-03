// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.qt.android.qtquickview_statuslistener;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import org.qtproject.qt.android.QtQmlStatus;
import org.qtproject.qt.android.QtQmlStatusChangeListener;
import org.qtproject.qt.android.QtQuickView;
import org.qtproject.qt.android.QtQuickViewContent;
import org.qtproject.qt.android.tst_qtquickview_statuslistener_qml.TestViewModule.TestView;

public class TestActivity extends Activity implements QtQmlStatusChangeListener
{
    native void jni_onQtQuickViewStatusChanged(int status);

    private final TestView m_testView = new TestView();
    private QtQuickView m_quickView;
    private QtQuickView m_testingView;
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

    public void loadQtQuickView(String qmlUri)
    {
        runOnUiThread(() -> {
            FrameLayout parent = findViewById(R.id.qmlFrame2);
            if (m_testingView != null)
                parent.removeView(m_testingView);

            m_testingView = new QtQuickView(this, qmlUri, "tst_qtquickview_statuslistener_qml");
            m_testingView.setStatusChangeListener(new QtQmlStatusChangeListener() {
                @Override
                public void onStatusChanged(QtQmlStatus status) {
                    jni_onQtQuickViewStatusChanged(status.ordinal());
                }
            });
            parent.addView(m_testingView,
                           new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                                                        ViewGroup.LayoutParams.MATCH_PARENT));
        });
    }

    public TestView testView() { return m_testView; }

    public static TestActivity instance() { return m_instance; }
}
