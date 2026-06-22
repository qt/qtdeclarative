// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQUICKSPINBOX_P_H
#define QQUICKSPINBOX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuickTemplates2/private/qquickabstractspinbox_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQml/qjsvalue.h>

QT_BEGIN_NAMESPACE

class QValidator;
class QQuickSpinBoxPrivate;
class QQuickIndicatorButton;

class Q_QUICKTEMPLATES2_EXPORT QQuickSpinBox : public QQuickControl,
                                               public QQuickAbstractSpinBox<QQuickSpinBox, int>
{
    Q_OBJECT
    Q_PROPERTY(int from READ from WRITE setFrom NOTIFY fromChanged FINAL)
    Q_PROPERTY(int to READ to WRITE setTo NOTIFY toChanged FINAL)
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(int stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged FINAL)
    Q_PROPERTY(bool editable READ isEditable WRITE setEditable NOTIFY editableChanged FINAL)
    Q_PROPERTY(bool live READ isLive WRITE setLive NOTIFY liveChanged FINAL REVISION(6, 6))

#if QT_CONFIG(validator)
    Q_PROPERTY(QValidator *validator READ validator WRITE setValidator NOTIFY validatorChanged FINAL)
#endif
    Q_PROPERTY(QJSValue textFromValue READ textFromValue WRITE setTextFromValue NOTIFY textFromValueChanged FINAL)
    Q_PROPERTY(QJSValue valueFromText READ valueFromText WRITE setValueFromText NOTIFY valueFromTextChanged FINAL)
    Q_PROPERTY(QQuickIndicatorButton *up READ up CONSTANT FINAL)
    Q_PROPERTY(QQuickIndicatorButton *down READ down CONSTANT FINAL)
    // 2.2 (Qt 5.9)
    Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ inputMethodHints WRITE setInputMethodHints NOTIFY inputMethodHintsChanged FINAL REVISION(2, 2))
    Q_PROPERTY(bool inputMethodComposing READ isInputMethodComposing NOTIFY inputMethodComposingChanged FINAL REVISION(2, 2))
    // 2.3 (Qt 5.10)
    Q_PROPERTY(bool wrap READ wrap WRITE setWrap NOTIFY wrapChanged FINAL REVISION(2, 3))
    // 2.4 (Qt 5.11)
    Q_PROPERTY(QString displayText READ displayText NOTIFY displayTextChanged FINAL REVISION(2, 4))
    QML_NAMED_ELEMENT(SpinBox)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickSpinBox(QQuickItem *parent = nullptr);
    ~QQuickSpinBox();

    void setFrom(int from);

    void setTo(int to);

    void setValue(int value);

    void setStepSize(int step);

    bool isLive() const;
    void setLive(bool live);

    QJSValue textFromValue() const;

    QJSValue valueFromText() const;

    void setWrap(bool wrap);

public Q_SLOTS:
    void increase() override;
    void decrease() override;

Q_SIGNALS:
    void fromChanged();
    void toChanged();
    void valueChanged();
    void stepSizeChanged();
    void editableChanged();
    Q_REVISION(6, 6) void liveChanged();
#if QT_CONFIG(validator)
    void validatorChanged();
#endif
    void textFromValueChanged();
    void valueFromTextChanged();
    // 2.2 (Qt 5.9)
    Q_REVISION(2, 2) void valueModified();
    Q_REVISION(2, 2) void inputMethodHintsChanged();
    Q_REVISION(2, 2) void inputMethodComposingChanged();
    // 2.3 (Qt 5.10)
    Q_REVISION(2, 3) void wrapChanged();
    // 2.4 (Qt 5.11)
    Q_REVISION(2, 4) void displayTextChanged();

protected:
    void focusInEvent(QFocusEvent *event) override
    {
        QQuickControl::focusInEvent(event);
        handleFocusInEvent(event);
    }
    void hoverEnterEvent(QHoverEvent *event) override
    {
        QQuickControl::hoverEnterEvent(event);
        handleHoverEnterEvent(event);
    }
    void hoverMoveEvent(QHoverEvent *event) override
    {
        QQuickControl::hoverMoveEvent(event);
        handleHoverMoveEvent(event);
    }
    void hoverLeaveEvent(QHoverEvent *event) override
    {
        QQuickControl::hoverLeaveEvent(event);
        handleHoverLeaveEvent(event);
    }
    void keyPressEvent(QKeyEvent *event) override
    {
        QQuickControl::keyPressEvent(event);
        handleKeyPressEvent(event);
    }
    void keyReleaseEvent(QKeyEvent *event) override
    {
        QQuickControl::keyReleaseEvent(event);
        handleKeyReleaseEvent(event);
    }
    void timerEvent(QTimerEvent *event) override
    {
        QQuickControl::timerEvent(event);
        handleTimerEvent(event);
    }
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override
    {
        QQuickControl::wheelEvent(event);
        handleWheelEvent(event);
    }
#endif
    void classBegin() override
    {
        QQuickControl::classBegin();
        handleClassBegin();
    }
    void componentComplete() override
    {
        QQuickControl::componentComplete();
        handleComponentComplete();
    }
    void itemChange(ItemChange change, const ItemChangeData &value) override
    {
        QQuickControl::itemChange(change, value);
        handleItemChange(change, value);
    }
    void contentItemChange(QQuickItem *newItem, QQuickItem *oldItem) override;
    void localeChange(const QLocale &newLocale, const QLocale &oldLocale) override
    {
        QQuickControl::localeChange(newLocale, oldLocale);
        handleLocaleChange();
    }
    QFont defaultFont() const override
    {
        return QQuickAbstractSpinBox<QQuickSpinBox, int>::defaultFont();
    }
#if QT_CONFIG(accessibility)
    QAccessible::Role accessibleRole() const override
    {
        return QQuickAbstractSpinBox<QQuickSpinBox, int>::accessibleRole();
    }
    void accessibilityActiveChanged(bool active) override
    {
        QQuickControl::accessibilityActiveChanged(active);
        handleAccessibilityActiveChanged(active);
    }
#endif

private:
    Q_DISABLE_COPY(QQuickSpinBox)
    Q_DECLARE_PRIVATE(QQuickSpinBox)
    friend class QQuickAbstractSpinBox<QQuickSpinBox, int>;
    QQuickControlPrivate *d_base_func();
    const QQuickControlPrivate *d_base_func() const;
};

QT_END_NAMESPACE

#endif // QQUICKSPINBOX_P_H
