// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
package com.example.qtabstractitemmodel_java;

import android.os.Bundle;
import android.util.Log;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import androidx.appcompat.app.AppCompatActivity;

import org.qtproject.qt.android.QtQmlStatus;
import org.qtproject.qt.android.QtQuickView;
import org.qtproject.qt.android.QtQmlStatusChangeListener;
import org.qtproject.example.qtabstractitemmodel.QmlModule.Main;

public class MainActivity extends AppCompatActivity implements QtQmlStatusChangeListener {
    private static final String TAG = "QtAIM MainActivity";
    private Main m_mainQmlContent;
    //! [1]
    private final MyDataModel m_model = new MyDataModel();
    //! [1]
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        m_mainQmlContent = new Main();
        QtQuickView qtQuickView = new QtQuickView(this);
        m_mainQmlContent.setStatusChangeListener(this);

        ViewGroup.LayoutParams params = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        FrameLayout qmlFrameLayout = findViewById(R.id.qmlFrame);
        qmlFrameLayout.addView(qtQuickView, params);

        //! [2]
        Button addRowAtEndButton = findViewById(R.id.addRowAtEndButton);
        Button removeRowFromEndButton = findViewById(R.id.removeRowFromEndButton);
        Button addColumnButton = findViewById(R.id.addColumnButton);
        Button removeLastColumnButton = findViewById(R.id.removeLastColumnButton);
        // Calls in a context of a Android main thread.
        addRowAtEndButton.setOnClickListener(view -> {
            m_model.addRow();
        });
        removeRowFromEndButton.setOnClickListener(view -> {
            m_model.removeRow();
        });
        addColumnButton.setOnClickListener(view -> {
            m_model.addColumn();
        });
        removeLastColumnButton.setOnClickListener(view -> {
            m_model.removeColumn();
        });
        //! [2]

        //! [3]
        qtQuickView.loadContent(m_mainQmlContent);
        //! [3]

    }

    //! [4]
    @Override
    public void onStatusChanged(QtQmlStatus qtQmlStatus) {
        Log.i(TAG, "Status of QtQuickView: " + qtQmlStatus);
        if (qtQmlStatus == QtQmlStatus.READY)
            // Calls in a context of a Android main thread.
            m_mainQmlContent.setDataModel(m_model);
    }
    //! [4]

}
