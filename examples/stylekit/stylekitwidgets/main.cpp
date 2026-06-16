// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QSlider>
#include <QButtonGroup>
#include <QProgressBar>
#include <QComboBox>
#include <QSpinBox>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyleKitStyle>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    struct StylePreset { const char *name; const char *path; };
    const StylePreset presets[] = {
        { "Classic", ":/ClassicStyle.qml" },
        { "Flat",    ":/FlatStyle.qml"    },
        { "Neon",    ":/NeonStyle.qml"    },
    };

    QStyleKitStyle *style = new QStyleKitStyle(QString::fromUtf8(presets[0].path));
    QApplication::setStyle(style);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget *scrollWidget = new QWidget();
    scrollArea->setWidget(scrollWidget);

    QVBoxLayout *contentLayout = new QVBoxLayout(scrollWidget);
    contentLayout->setAlignment(Qt::AlignTop);

    // Buttons
    QGroupBox *buttonsGroup = new QGroupBox("Buttons");
    QHBoxLayout *buttonsLayout = new QHBoxLayout(buttonsGroup);
    QPushButton *normalButton = new QPushButton("Normal");
    QPushButton *checkableButton = new QPushButton("Checkable");
    checkableButton->setCheckable(true);
    QPushButton *disabledButton = new QPushButton("Disabled");
    disabledButton->setEnabled(false);
    QPushButton *flatButton = new QPushButton("Flat");
    flatButton->setFlat(true);
    buttonsLayout->addWidget(normalButton);
    buttonsLayout->addWidget(checkableButton);
    buttonsLayout->addWidget(disabledButton);
    buttonsLayout->addWidget(flatButton);
    buttonsLayout->addStretch();
    contentLayout->addWidget(buttonsGroup);

    // CheckBoxes and RadioButtons
    QGroupBox *checkRadioGroup = new QGroupBox("CheckBoxes and RadioButtons");
    QGridLayout *checkRadioLayout = new QGridLayout(checkRadioGroup);
    checkRadioLayout->setColumnStretch(3, 1);

    QCheckBox *checkBox1 = new QCheckBox("Mango");
    checkBox1->setChecked(true);
    QCheckBox *checkBox2 = new QCheckBox("Avocado");
    QCheckBox *checkBox3 = new QCheckBox("Banano");
    checkBox3->setChecked(true);

    QRadioButton *radioButton1 = new QRadioButton("Pasta");
    QRadioButton *radioButton2 = new QRadioButton("Lasagna");
    radioButton2->setChecked(true);
    QRadioButton *radioButton3 = new QRadioButton("Burrita");

    QButtonGroup *radioGroup = new QButtonGroup(scrollWidget);
    radioGroup->addButton(radioButton1);
    radioGroup->addButton(radioButton2);
    radioGroup->addButton(radioButton3);

    checkRadioLayout->addWidget(checkBox1,    0, 0);
    checkRadioLayout->addWidget(checkBox2,    0, 1);
    checkRadioLayout->addWidget(checkBox3,    0, 2);
    checkRadioLayout->addWidget(radioButton1, 1, 0);
    checkRadioLayout->addWidget(radioButton2, 1, 1);
    checkRadioLayout->addWidget(radioButton3, 1, 2);
    contentLayout->addWidget(checkRadioGroup);

    // Text Inputs
    QGroupBox *textInputsGroup = new QGroupBox("Text Inputs");
    QHBoxLayout *textInputsLayout = new QHBoxLayout(textInputsGroup);
    QLineEdit *lineEdit1 = new QLineEdit();
    lineEdit1->setPlaceholderText("Potato");
    QLineEdit *lineEdit2 = new QLineEdit();
    lineEdit2->setPlaceholderText("Tomato");
    textInputsLayout->addWidget(lineEdit1);
    textInputsLayout->addWidget(lineEdit2);
    contentLayout->addWidget(textInputsGroup);

    // Misc
    QGroupBox *miscGroup = new QGroupBox("Misc");
    QHBoxLayout *miscLayout = new QHBoxLayout(miscGroup);
    QSpinBox *spinBox = new QSpinBox();
    spinBox->setRange(0, 100);
    spinBox->setValue(42);
    QComboBox *comboBox = new QComboBox();
    comboBox->addItems({ "One", "February", "Aramis", "Winter", "Friday" });
    miscLayout->addWidget(spinBox);
    miscLayout->addWidget(comboBox);
    miscLayout->addStretch();
    contentLayout->addWidget(miscGroup);

    // Sliders
    QGroupBox *slidersGroup = new QGroupBox("Sliders");
    slidersGroup->setMinimumHeight(250);
    QHBoxLayout *slidersLayout = new QHBoxLayout(slidersGroup);
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setValue(50);
    QSlider *verticalSlider = new QSlider(Qt::Vertical);
    verticalSlider->setRange(0, 100);
    verticalSlider->setValue(30);
    slidersLayout->addWidget(slider);
    slidersLayout->addWidget(verticalSlider);
    slidersLayout->addStretch();
    contentLayout->addWidget(slidersGroup);

    // Progress Bar
    QGroupBox *progressGroup = new QGroupBox("Progress Bar");
    QHBoxLayout *progressLayout = new QHBoxLayout(progressGroup);
    QProgressBar *progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(20);
    progressLayout->addWidget(progressBar);
    contentLayout->addWidget(progressGroup);

    // --- Settings panel ---

    QGroupBox *settingsGroup = new QGroupBox("Settings");
    settingsGroup->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QFormLayout *settingsLayout = new QFormLayout(settingsGroup);

    QComboBox *stylePathCombo = new QComboBox();
    for (const StylePreset &preset : presets)
        stylePathCombo->addItem(QString::fromUtf8(preset.name), QString::fromUtf8(preset.path));
    stylePathCombo->setCurrentIndex(0);
    settingsLayout->addRow("Style", stylePathCombo);

    QComboBox *themeCombo = new QComboBox();
    settingsLayout->addRow("Theme", themeCombo);

    auto refreshThemes = [style, themeCombo]() {
        const QStringList names = style->themeNames();
        const QString current = style->themeName();
        const QSignalBlocker blocker(themeCombo);
        themeCombo->clear();
        themeCombo->addItems(names);
        const int idx = themeCombo->findText(current);
        if (idx >= 0)
            themeCombo->setCurrentIndex(idx);
    };
    refreshThemes();

    auto applyStylePath = [style, refreshThemes](const QString &path) {
        style->setStylePath(path);
        refreshThemes();
    };

    QObject::connect(stylePathCombo, &QComboBox::activated, &app,
                     [stylePathCombo, applyStylePath](int index) {
                         const QString data = stylePathCombo->itemData(index).toString();
                         applyStylePath(data.isEmpty() ? stylePathCombo->itemText(index) : data);
                     });
    QObject::connect(themeCombo, &QComboBox::activated, &app,
                     [style, themeCombo](int) {
                         style->setThemeName(themeCombo->currentText());
                     });

    // --- Main window ---

    QWidget window;
    window.setWindowTitle("StyleKit Widgets Example");
    window.resize(800, 600);

    QHBoxLayout *windowLayout = new QHBoxLayout(&window);
    windowLayout->addWidget(scrollArea);
    windowLayout->addWidget(settingsGroup);

    window.show();

    return app.exec();
}
