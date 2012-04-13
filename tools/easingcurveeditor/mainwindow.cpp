/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "splineeditor.h"
#include <QtQuick/QQuickView>
#include <QtQuick>
#include <QtQml/QQmlContext>
#include <QEasingCurve>
#include <QHBoxLayout>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
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
    connect(splineEditor, SIGNAL(easingCurveCodeChanged(QString)), ui_properties.plainTextEdit, SLOT(setPlainText(QString)));

    quickView.rootContext()->setContextProperty(QLatin1String("spinBox"), ui_properties.spinBox);

    foreach (const QString &name, splineEditor->presetNames())
        ui_properties.comboBox->addItem(name);

    connect(ui_properties.comboBox, SIGNAL(currentIndexChanged(QString)), splineEditor, SLOT(setPreset(QString)));

    splineEditor->setPreset(ui_properties.comboBox->currentText());

    QVBoxLayout *groupBoxLayout = new QVBoxLayout(ui_properties.groupBox);
    groupBoxLayout->setMargin(0);
    ui_properties.groupBox->setLayout(groupBoxLayout);

    groupBoxLayout->addWidget(splineEditor->pointListWidget());
    m_splineEditor = splineEditor;
    connect(ui_properties.plainTextEdit, SIGNAL(textChanged()), this, SLOT(textEditTextChanged()));
    initQml();
}

void MainWindow::showQuickView()
{
    const int margin = 16;
    quickView.setPos(pos() + QPoint(0, frameGeometry().height() + margin));

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
    quickView.setWindowFlags(Qt::FramelessWindowHint);
    quickView.rootContext()->setContextProperty(QLatin1String("editor"), m_splineEditor);
    quickView.setSource(QUrl("qrc:/preview.qml"));
    quickView.show();
}
