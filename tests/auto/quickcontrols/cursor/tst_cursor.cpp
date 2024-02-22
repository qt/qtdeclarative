// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtQuick/qquickview.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuickTemplates2/private/qquickpageindicator_p.h>
#include <QtQuickTemplates2/private/qquickscrollbar_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickControls2/qquickstyle.h>

#if QT_CONFIG(cursor)
#  include <QtGui/qscreen.h>
#  include <QtGui/qcursor.h>
#endif

using namespace QQuickControlsTestUtils;

class tst_cursor : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_cursor();

private slots:
    void init() override;
    void controls_data();
    void controls();
    void editable();
    void pageIndicator();
    void scrollBar();
    void textArea();
};

tst_cursor::tst_cursor()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    QQuickStyle::setStyle("Basic");
}

void tst_cursor::init()
{
    QQmlDataTest::init();

#if QT_CONFIG(cursor)
    // Ensure mouse cursor was not left by previous tests where widgets
    // will appear, as it could cause events and interfere with the tests.
    const QScreen *screen = QGuiApplication::primaryScreen();
    const QRect availableGeometry = screen->availableGeometry();
    QCursor::setPos(availableGeometry.topLeft());
#endif
}

void tst_cursor::controls_data()
{
    QTest::addColumn<QString>("testFile");

    QTest::newRow("buttons") << "buttons.qml";
    QTest::newRow("containers") << "containers.qml";
    QTest::newRow("sliders") << "sliders.qml";
}

void tst_cursor::controls()
{
    QFETCH(QString, testFile);

    QQuickView view(testFileUrl(testFile));
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickItem *mouseArea = view.rootObject();
    QVERIFY(mouseArea);
    QCOMPARE(mouseArea->cursor().shape(), Qt::ForbiddenCursor);

    QQuickItem *column = mouseArea->childItems().value(0);
    QVERIFY(column);

    const auto controls = column->childItems();
    for (QQuickItem *control : controls) {
        QCOMPARE(control->cursor().shape(), Qt::ArrowCursor);

        QTest::mouseMove(&view, control->mapToScene(QPointF(-1, -1)).toPoint());
        QCOMPARE(view.cursor().shape(), Qt::ForbiddenCursor);

        QTest::mouseMove(&view, control->mapToScene(QPointF(0, 0)).toPoint());
#ifndef Q_OS_WEBOS
        //webOS cursor handling uses BitmapCursor for ArrowCursor
        QCOMPARE(view.cursor().shape(), Qt::ArrowCursor);
#endif

        QTest::mouseMove(&view, control->mapToScene(QPointF(control->width() + 1, control->height() + 1)).toPoint());
        QCOMPARE(view.cursor().shape(), Qt::ForbiddenCursor);
    }
}

void tst_cursor::editable()
{
    QQuickView view(testFileUrl("editable.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickItem *mouseArea = view.rootObject();
    QVERIFY(mouseArea);
    QCOMPARE(mouseArea->cursor().shape(), Qt::ForbiddenCursor);

    QQuickItem *column = mouseArea->childItems().value(0);
    QVERIFY(column);

    const auto children = column->childItems();
    for (QQuickItem *child : children) {
        QQuickControl *control = qobject_cast<QQuickControl *>(child);
        QVERIFY(control);
        QCOMPARE(control->cursor().shape(), Qt::ArrowCursor);
        QCOMPARE(control->contentItem()->cursor().shape(), Qt::IBeamCursor);

        QTest::mouseMove(&view, control->mapToScene(QPointF(-1, -1)).toPoint());
        QCOMPARE(view.cursor().shape(), Qt::ForbiddenCursor);

        QTest::mouseMove(&view, control->mapToScene(QPointF(control->width() / 2, control->height() / 2)).toPoint());
        QCOMPARE(view.cursor().shape(), Qt::IBeamCursor);

        control->setProperty("editable", false);
        QCOMPARE(control->cursor().shape(), Qt::ArrowCursor);
        QCOMPARE(control->contentItem()->cursor().shape(), Qt::ArrowCursor);
#ifndef Q_OS_WEBOS
        //webOS cursor handling uses BitmapCursor for ArrowCursor
        QCOMPARE(view.cursor().shape(), Qt::ArrowCursor);
#endif

        QTest::mouseMove(&view, control->mapToScene(QPointF(control->width() + 1, control->height() + 1)).toPoint());
        QCOMPARE(view.cursor().shape(), Qt::ForbiddenCursor);
    }
}

void tst_cursor::pageIndicator()
{
    QQuickView view(testFileUrl("pageindicator.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickItem *mouseArea = view.rootObject();
    QVERIFY(mouseArea);
    QCOMPARE(mouseArea->cursor().shape(), Qt::ForbiddenCursor);

    QQuickPageIndicator *indicator = qobject_cast<QQuickPageIndicator *>(mouseArea->childItems().value(0));
    QVERIFY(indicator);

    QTest::mouseMove(&view, indicator->mapToScene(QPointF(-1, -1)).toPoint());
    QCOMPARE(view.cursor().shape(), Qt::ForbiddenCursor);

    QTest::mouseMove(&view, indicator->mapToScene(QPointF(0, 0)).toPoint());
    QCOMPARE(view.cursor().shape(), Qt::ForbiddenCursor);

    indicator->setInteractive(true);
#ifndef Q_OS_WEBOS
    //webOS cursor handling uses BitmapCursor for ArrowCursor
    QCOMPARE(view.cursor().shape(), Qt::ArrowCursor);
#endif

    QTest::mouseMove(&view, indicator->mapToScene(QPointF(indicator->width() + 1, indicator->height() + 1)).toPoint());
    QCOMPARE(view.cursor().shape(), Qt::ForbiddenCursor);
}

// QTBUG-59629
void tst_cursor::scrollBar()
{
    // Ensure that the mouse cursor has the correct shape when over a scrollbar
    // which is itself over a text area with IBeamCursor.
    QQuickControlsApplicationHelper helper(this, QStringLiteral("scrollbar.qml"));
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QQuickScrollBar *scrollBar = helper.appWindow->property("scrollBar").value<QQuickScrollBar*>();
    QVERIFY(scrollBar);

    QQuickTextArea *textArea = helper.appWindow->property("textArea").value<QQuickTextArea*>();
    QVERIFY(textArea);

    textArea->setText(QString("\n").repeated(100));

    const QPoint textAreaPos(window->width() / 2, window->height() / 2);
    QTest::mouseMove(window, textAreaPos);
    QCOMPARE(window->cursor().shape(), textArea->cursor().shape());
    QCOMPARE(textArea->cursor().shape(), Qt::CursorShape::IBeamCursor);

    const QPoint scrollBarPos(window->width() - scrollBar->width() / 2, window->height() / 2);
    QTest::mouseMove(window, scrollBarPos);

    QVERIFY(scrollBar->isActive());
#ifndef Q_OS_WEBOS
    //webOS cursor handling uses BitmapCursor for ArrowCursor
    QCOMPARE(window->cursor().shape(), scrollBar->cursor().shape());
#endif
    QCOMPARE(scrollBar->cursor().shape(), Qt::CursorShape::ArrowCursor);

    scrollBar->setInteractive(false);
    QCOMPARE(window->cursor().shape(), textArea->cursor().shape());
}

// QTBUG-104089
void tst_cursor::textArea()
{
    QQuickTextArea textArea;
    QCOMPARE(textArea.cursor().shape(), Qt::IBeamCursor);

    textArea.setReadOnly(true);
    QVERIFY(textArea.selectByMouse());
    QCOMPARE(textArea.cursor().shape(), Qt::IBeamCursor);

    textArea.setSelectByMouse(false);
    QCOMPARE(textArea.cursor().shape(), Qt::ArrowCursor);
}

QTEST_MAIN(tst_cursor)

#include "tst_cursor.moc"
