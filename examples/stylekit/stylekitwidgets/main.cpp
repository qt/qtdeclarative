// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QStyleKitStyle>

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <QIcon>
#include <QKeySequence>

#include <algorithm>
#include <array>

using namespace Qt::StringLiterals;

struct StylePreset
{
    QLatin1StringView name;
    QLatin1StringView path;
};

static constexpr std::array<StylePreset, 3> presets = {
    StylePreset{ "Classic"_L1, ":/ClassicStyle.qml"_L1 },
    StylePreset{ "Flat"_L1,    ":/FlatStyle.qml"_L1    },
    StylePreset{ "Neon"_L1,    ":/NeonStyle.qml"_L1    },
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
    QGroupBox(tr("Settings"), parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    auto *settingsLayout = new QFormLayout(this);

    for (const StylePreset &preset : presets)
        m_stylePathCombo->addItem(preset.name, preset.path);
    m_stylePathCombo->setCurrentIndex(0);

    settingsLayout->addRow(tr("Style"), m_stylePathCombo);
    settingsLayout->addRow(tr("Theme"), m_themeCombo);

    if (m_style != nullptr)
        refreshThemes();
    else
        setEnabled(false);

    connect(m_stylePathCombo, &QComboBox::activated, this, &StyleControl::stylePathComboActivated);
    connect(m_themeCombo, &QComboBox::activated, this, &StyleControl::themeComboActivated);
}

void StyleControl::refreshThemes()
{
    const QStringList names = m_style->availableThemeNames();
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

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
};

MainWindow::MainWindow()
{
    menuBar()->setNativeMenuBar(false);
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::ApplicationExit), tr("Quit"),
                        QKeySequence::Quit, this, &QWidget::close);
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::HelpAbout), tr("About Qt"),
                        QKeySequence::HelpContents, this, QApplication::aboutQt);

    auto *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *scrollWidget = new QWidget();
    scrollArea->setWidget(scrollWidget);

    auto *contentLayout = new QVBoxLayout(scrollWidget);
    contentLayout->setAlignment(Qt::AlignTop);

    // Buttons
    auto *buttonsGroup = new QGroupBox(tr("Buttons"));
    auto *buttonsLayout = new QHBoxLayout(buttonsGroup);
    auto *normalButton = new QPushButton(tr("Normal"));
    auto *checkableButton = new QPushButton(tr("Checkable"));
    checkableButton->setCheckable(true);
    auto *disabledButton = new QPushButton(tr("Disabled"));
    disabledButton->setEnabled(false);
    auto *flatButton = new QPushButton(tr("Flat"));
    flatButton->setFlat(true);
    buttonsLayout->addWidget(normalButton);
    buttonsLayout->addWidget(checkableButton);
    buttonsLayout->addWidget(disabledButton);
    buttonsLayout->addWidget(flatButton);
    buttonsLayout->addStretch();
    contentLayout->addWidget(buttonsGroup);

    // CheckBoxes and RadioButtons
    auto *checkRadioGroup = new QGroupBox(tr("CheckBoxes and RadioButtons"));
    auto *checkRadioLayout = new QGridLayout(checkRadioGroup);
    checkRadioLayout->setColumnStretch(3, 1);

    auto *checkBox1 = new QCheckBox(tr("Mango"));
    checkBox1->setChecked(true);
    auto *checkBox2 = new QCheckBox(tr("Avocado"));
    auto *checkBox3 = new QCheckBox(tr("Banano"));
    checkBox3->setChecked(true);

    auto *radioButton1 = new QRadioButton(tr("Pasta"));
    auto *radioButton2 = new QRadioButton(tr("Lasagna"));
    radioButton2->setChecked(true);
    auto *radioButton3 = new QRadioButton(tr("Burrita"));

    auto *radioGroup = new QButtonGroup(scrollWidget);
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
    auto *textInputsGroup = new QGroupBox(tr("Text Inputs"));
    auto *textInputsLayout = new QHBoxLayout(textInputsGroup);
    auto *lineEdit1 = new QLineEdit();
    lineEdit1->setPlaceholderText(tr("Potato"));
    auto *lineEdit2 = new QLineEdit();
    lineEdit2->setPlaceholderText(tr("Tomato"));
    textInputsLayout->addWidget(lineEdit1);
    textInputsLayout->addWidget(lineEdit2);
    contentLayout->addWidget(textInputsGroup);

    // Misc
    auto *miscGroup = new QGroupBox(tr("Misc"));
    auto *miscLayout = new QHBoxLayout(miscGroup);
    auto *spinBox = new QSpinBox();
    spinBox->setRange(0, 100);
    spinBox->setValue(42);
    auto *comboBox = new QComboBox();
    comboBox->addItems({ tr("One"), tr("February"), tr("Aramis"), tr("Winter"), tr("Friday") });
    miscLayout->addWidget(spinBox);
    miscLayout->addWidget(comboBox);
    miscLayout->addStretch();
    contentLayout->addWidget(miscGroup);

    // Sliders
    auto *slidersGroup = new QGroupBox(tr("Sliders"));
    slidersGroup->setMinimumHeight(250);
    auto *slidersLayout = new QHBoxLayout(slidersGroup);
    auto *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setValue(50);
    auto *verticalSlider = new QSlider(Qt::Vertical);
    verticalSlider->setRange(0, 100);
    verticalSlider->setValue(30);
    slidersLayout->addWidget(slider);
    slidersLayout->addWidget(verticalSlider);
    slidersLayout->addStretch();
    contentLayout->addWidget(slidersGroup);

    // Progress Bar
    auto *progressGroup = new QGroupBox(tr("Progress Bar"));
    auto *progressLayout = new QHBoxLayout(progressGroup);
    auto *progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(20);
    progressLayout->addWidget(progressBar);
    contentLayout->addWidget(progressGroup);

    auto *centralWidget = new QWidget();
    auto *centralLayout = new QHBoxLayout(centralWidget);
    centralLayout->addWidget(scrollArea);
    centralLayout->addWidget(new StyleControl(centralWidget));
    setCentralWidget(centralWidget);
}

static bool isStyleOption(const char *option)
{
    return qstrcmp(option, "-style") == 0;
}

int main(int argc, char *argv[])
{
    const bool hasStyleOption = std::any_of(argv + 1, argv + argc, isStyleOption);

    QApplication app(argc, argv);

    auto *style = new QStyleKitStyle(presets[0].path);
    if (!hasStyleOption)
        QApplication::setStyle(style);

    // --- Main window ---

    MainWindow window;
    window.setWindowTitle(MainWindow::tr("StyleKit Widgets Example"));
    window.resize(800, 600);

    window.show();

    return QApplication::exec();
}

#include "main.moc"
