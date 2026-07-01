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

struct StylePreset
{
    const char *name;
    const char *path;
};

static const StylePreset presets[] = {
    { "Classic", ":/ClassicStyle.qml" },
    { "Flat",    ":/FlatStyle.qml"    },
    { "Neon",    ":/NeonStyle.qml"    },
};

class StyleControl : public QGroupBox
{
    Q_OBJECT
public:
    explicit StyleControl(QWidget *parent = nullptr);

private slots:
    void stylePathComboActivated(int index);
    void themeComboActivated(int);

private:
    void refreshThemes();
    void setStylePath(const QString &path);

    QComboBox *m_themeCombo = new QComboBox();
    QComboBox *m_stylePathCombo = new QComboBox();
    QStyleKitStyle *m_style = qobject_cast<QStyleKitStyle *>(QApplication::style());
};

StyleControl::StyleControl(QWidget *parent) :
    QGroupBox("Settings", parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QFormLayout *settingsLayout = new QFormLayout(this);

    for (const StylePreset &preset : presets)
        m_stylePathCombo->addItem(QString::fromUtf8(preset.name), QString::fromUtf8(preset.path));
    m_stylePathCombo->setCurrentIndex(0);

    settingsLayout->addRow("Style", m_stylePathCombo);
    settingsLayout->addRow("Theme", m_themeCombo);

    if (m_style != nullptr)
        refreshThemes();
    else
        setEnabled(false);

    connect(m_stylePathCombo, &QComboBox::activated, this, &StyleControl::stylePathComboActivated);
    connect(m_themeCombo, &QComboBox::activated, this, &StyleControl::themeComboActivated);
}

void StyleControl::refreshThemes()
{
    const QStringList names = m_style->themeNames();
    const QString current = m_style->themeName();
    const QSignalBlocker blocker(m_themeCombo);
    m_themeCombo->clear();
    m_themeCombo->addItems(names);
    const int idx = m_themeCombo->findText(current);
    if (idx >= 0)
        m_themeCombo->setCurrentIndex(idx);
}

void StyleControl::setStylePath(const QString &path)
{
    m_style->setStylePath(path);
    refreshThemes();
}

void StyleControl::stylePathComboActivated(int index)
{
    const QString data = m_stylePathCombo->itemData(index).toString();
    setStylePath(data.isEmpty() ? m_stylePathCombo->itemText(index) : data);
}

void StyleControl::themeComboActivated(int)
{
    m_style->setThemeName(m_themeCombo->currentText());
}

class Widget : public QWidget
{
    Q_OBJECT
public:
    Widget();
};

Widget::Widget()
{
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

    auto *windowLayout = new QHBoxLayout(this);
    windowLayout->addWidget(scrollArea);
    windowLayout->addWidget(new StyleControl(this));
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QStyleKitStyle *style = new QStyleKitStyle(QString::fromUtf8(presets[0].path));
    QApplication::setStyle(style);

    // --- Main window ---

    Widget window;
    window.setWindowTitle("StyleKit Widgets Example");
    window.resize(800, 600);

    window.show();

    return app.exec();
}

#include "main.moc"
