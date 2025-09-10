// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mainwindow.h"
#include "splineeditor.h"
#include <QtQuick/QQuickView>
#include <QtQml/QQmlContext>
#include <QEasingCurve>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDoubleValidator>
#include <QDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setWindowTitle("QML Easing Curve Editor");
    SplineEditor *splineEditor = new SplineEditor(this);

    QWidget *mainWidget = new QWidget(this);

    setCentralWidget(mainWidget);

    QHBoxLayout *hboxLayout = new QHBoxLayout(mainWidget);
    QVBoxLayout *vboxLayout = new QVBoxLayout();

    mainWidget->setLayout(hboxLayout);
    hboxLayout->addLayout(vboxLayout);

    QWidget *propertyWidget = new QWidget(this);
    ui_properties.setupUi(propertyWidget);

    ui_properties.spinBox->setMinimum(50);
    ui_properties.spinBox->setMaximum(10000);
    ui_properties.spinBox->setValue(500);

    hboxLayout->addWidget(propertyWidget);

    m_placeholder = new QWidget(this);

    m_placeholder->setFixedSize(quickView.size());

    vboxLayout->addWidget(splineEditor);
    vboxLayout->addWidget(m_placeholder);

    ui_properties.plainTextEdit->setPlainText(splineEditor->generateCode());
    connect(splineEditor, &SplineEditor::easingCurveCodeChanged, ui_properties.plainTextEdit, &QPlainTextEdit::setPlainText);

    quickView.rootContext()->setContextProperty(QLatin1String("spinBox"), ui_properties.spinBox);

    const auto presetNames = splineEditor->presetNames();
    for (const QString &name : presetNames)
        ui_properties.comboBox->addItem(name);

    connect(ui_properties.comboBox, &QComboBox::currentTextChanged, splineEditor, &SplineEditor::setPreset);

    splineEditor->setPreset(ui_properties.comboBox->currentText());

    QVBoxLayout *groupBoxLayout = new QVBoxLayout(ui_properties.groupBox);
    groupBoxLayout->setContentsMargins(QMargins());
    ui_properties.groupBox->setLayout(groupBoxLayout);

    groupBoxLayout->addWidget(splineEditor->pointListWidget());
    m_splineEditor = splineEditor;
    connect(ui_properties.plainTextEdit, &QPlainTextEdit::textChanged, this, &MainWindow::textEditTextChanged);

    QDialog* importDialog = new QDialog(this);
    ui_import.setupUi(importDialog);
    ui_import.inInfluenceEdit->setValidator(new QDoubleValidator(this));
    ui_import.inSlopeEdit->setValidator(new QDoubleValidator(this));
    ui_import.outInfluenceEdit->setValidator(new QDoubleValidator(this));
    ui_import.outSlopeEdit->setValidator(new QDoubleValidator(this));
    connect(ui_properties.importButton, &QPushButton::clicked, importDialog, &QDialog::show);
    connect(importDialog, &QDialog::finished, this, &MainWindow::importData);

    initQml();
}

void MainWindow::showQuickView()
{
    const int margin = 16;
    quickView.setPosition(pos() + QPoint(0, frameGeometry().height() + margin));

    quickView.raise();
    quickView.show();
}

void MainWindow::textEditTextChanged()
{
    m_splineEditor->setEasingCurve(ui_properties.plainTextEdit->toPlainText().trimmed());
}

void MainWindow::moveEvent(QMoveEvent *event)
{
    QMainWindow::moveEvent(event);
    showQuickView();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    showQuickView();
}

void MainWindow::initQml()
{
    quickView.setFlags(Qt::FramelessWindowHint);
    quickView.rootContext()->setContextProperty(QLatin1String("editor"), m_splineEditor);
    quickView.setSource(QUrl("qrc:/preview.qml"));
    quickView.show();
}

void MainWindow::closeEvent(QCloseEvent *)
{
    quickView.close();
}

void MainWindow::importData(int result)
{
    if (!result)
        return;
    double ii = ui_import.inInfluenceEdit->text().toDouble();
    double is = ui_import.inSlopeEdit->text().toDouble();
    double oi = ui_import.outInfluenceEdit->text().toDouble();
    double os = ui_import.outSlopeEdit->text().toDouble();
    ii = qBound<double>(0., ii, 100.) / 100.;
    oi = qBound<double>(0., oi, 100.) / 100.;
    QString generatedString = QString("[%1,%2,%3,%4,1,1]").arg(ii, 0, 'f', 3)
        .arg(ii*is,0,'f',3).arg(1-oi, 0, 'f', 3).arg(1-(oi*os), 0, 'f', 3);
    ui_properties.plainTextEdit->setPlainText(generatedString);
}

#include "moc_mainwindow.cpp"
