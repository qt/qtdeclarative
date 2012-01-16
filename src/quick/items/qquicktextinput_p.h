// Commit: 2f173e4945dd8414636c1061acfaf9c2d8b718d8
/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKTEXTINPUT_P_H
#define QQUICKTEXTINPUT_P_H

#include "qquickimplicitsizeitem_p.h"
#include <QtGui/qtextoption.h>
#include <QtGui/qvalidator.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickTextInputPrivate;
class QValidator;
class Q_AUTOTEST_EXPORT QQuickTextInput : public QQuickImplicitSizeItem
{
    Q_OBJECT
    Q_ENUMS(HAlignment)
    Q_ENUMS(VAlignment)
    Q_ENUMS(WrapMode)
    Q_ENUMS(EchoMode)
    Q_ENUMS(SelectionMode)
    Q_ENUMS(CursorPosition)

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(int length READ length NOTIFY textChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(QColor selectedTextColor READ selectedTextColor WRITE setSelectedTextColor NOTIFY selectedTextColorChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(HAlignment horizontalAlignment READ hAlign WRITE setHAlign RESET resetHAlign NOTIFY horizontalAlignmentChanged)
    Q_PROPERTY(HAlignment effectiveHorizontalAlignment READ effectiveHAlign NOTIFY effectiveHorizontalAlignmentChanged)
    Q_PROPERTY(VAlignment verticalAlignment READ vAlign WRITE setVAlign NOTIFY verticalAlignmentChanged)
    Q_PROPERTY(WrapMode wrapMode READ wrapMode WRITE setWrapMode NOTIFY wrapModeChanged)

    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(bool cursorVisible READ isCursorVisible WRITE setCursorVisible NOTIFY cursorVisibleChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(QRect cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged)
    Q_PROPERTY(QDeclarativeComponent *cursorDelegate READ cursorDelegate WRITE setCursorDelegate NOTIFY cursorDelegateChanged)
    Q_PROPERTY(int selectionStart READ selectionStart NOTIFY selectionStartChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd NOTIFY selectionEndChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectedTextChanged)

    Q_PROPERTY(int maximumLength READ maxLength WRITE setMaxLength NOTIFY maximumLengthChanged)
#ifndef QT_NO_VALIDATOR
    Q_PROPERTY(QValidator* validator READ validator WRITE setValidator NOTIFY validatorChanged)
#endif
    Q_PROPERTY(QString inputMask READ inputMask WRITE setInputMask NOTIFY inputMaskChanged)
    Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ imHints WRITE setIMHints)

    Q_PROPERTY(bool acceptableInput READ hasAcceptableInput NOTIFY acceptableInputChanged)
    Q_PROPERTY(EchoMode echoMode READ echoMode WRITE setEchoMode NOTIFY echoModeChanged)
    Q_PROPERTY(bool activeFocusOnPress READ focusOnPress WRITE setFocusOnPress NOTIFY activeFocusOnPressChanged)
    Q_PROPERTY(QString passwordCharacter READ passwordCharacter WRITE setPasswordCharacter NOTIFY passwordCharacterChanged)
    Q_PROPERTY(QString displayText READ displayText NOTIFY displayTextChanged)
    Q_PROPERTY(bool autoScroll READ autoScroll WRITE setAutoScroll NOTIFY autoScrollChanged)
    Q_PROPERTY(bool selectByMouse READ selectByMouse WRITE setSelectByMouse NOTIFY selectByMouseChanged)
    Q_PROPERTY(SelectionMode mouseSelectionMode READ mouseSelectionMode WRITE setMouseSelectionMode NOTIFY mouseSelectionModeChanged)
    Q_PROPERTY(bool canPaste READ canPaste NOTIFY canPasteChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)
    Q_PROPERTY(bool inputMethodComposing READ isInputMethodComposing NOTIFY inputMethodComposingChanged)

public:
    QQuickTextInput(QQuickItem * parent=0);
    ~QQuickTextInput();

    void componentComplete();

    enum EchoMode {//To match QLineEdit::EchoMode
        Normal,
        NoEcho,
        Password,
        PasswordEchoOnEdit
    };

    enum HAlignment {
        AlignLeft = Qt::AlignLeft,
        AlignRight = Qt::AlignRight,
        AlignHCenter = Qt::AlignHCenter
    };

    enum VAlignment {
        AlignTop = Qt::AlignTop,
        AlignBottom = Qt::AlignBottom,
        AlignVCenter = Qt::AlignVCenter
    };

    enum WrapMode {
        NoWrap = QTextOption::NoWrap,
        WordWrap = QTextOption::WordWrap,
        WrapAnywhere = QTextOption::WrapAnywhere,
        WrapAtWordBoundaryOrAnywhere = QTextOption::WrapAtWordBoundaryOrAnywhere, // COMPAT
        Wrap = QTextOption::WrapAtWordBoundaryOrAnywhere
    };

    enum SelectionMode {
        SelectCharacters,
        SelectWords
    };

    enum CursorPosition {
        CursorBetweenCharacters,
        CursorOnCharacter
    };


    //Auxilliary functions needed to control the TextInput from QML
    Q_INVOKABLE void positionAt(QDeclarativeV8Function *args) const;
    Q_INVOKABLE QRectF positionToRectangle(int pos) const;
    Q_INVOKABLE void moveCursorSelection(int pos);
    Q_INVOKABLE void moveCursorSelection(int pos, SelectionMode mode);

    Q_INVOKABLE void openSoftwareInputPanel();
    Q_INVOKABLE void closeSoftwareInputPanel();

    QString text() const;
    void setText(const QString &);

    int length() const;

    QFont font() const;
    void setFont(const QFont &font);

    QColor color() const;
    void setColor(const QColor &c);

    QColor selectionColor() const;
    void setSelectionColor(const QColor &c);

    QColor selectedTextColor() const;
    void setSelectedTextColor(const QColor &c);

    HAlignment hAlign() const;
    void setHAlign(HAlignment align);
    void resetHAlign();
    HAlignment effectiveHAlign() const;

    VAlignment vAlign() const;
    void setVAlign(VAlignment align);

    WrapMode wrapMode() const;
    void setWrapMode(WrapMode w);

    bool isReadOnly() const;
    void setReadOnly(bool);

    bool isCursorVisible() const;
    void setCursorVisible(bool on);

    int cursorPosition() const;
    void setCursorPosition(int cp);

    QRect cursorRectangle() const;

    int selectionStart() const;
    int selectionEnd() const;

    QString selectedText() const;

    int maxLength() const;
    void setMaxLength(int ml);

#ifndef QT_NO_VALIDATOR
    QValidator * validator() const;
    void setValidator(QValidator* v);
#endif
    QString inputMask() const;
    void setInputMask(const QString &im);

    EchoMode echoMode() const;
    void setEchoMode(EchoMode echo);

    QString passwordCharacter() const;
    void setPasswordCharacter(const QString &str);

    QString displayText() const;

    QDeclarativeComponent* cursorDelegate() const;
    void setCursorDelegate(QDeclarativeComponent*);

    bool focusOnPress() const;
    void setFocusOnPress(bool);

    bool autoScroll() const;
    void setAutoScroll(bool);

    bool selectByMouse() const;
    void setSelectByMouse(bool);

    SelectionMode mouseSelectionMode() const;
    void setMouseSelectionMode(SelectionMode mode);

    bool hasAcceptableInput() const;

    QVariant inputMethodQuery(Qt::InputMethodQuery property) const;

    QRectF boundingRect() const;
    bool canPaste() const;

    bool canUndo() const;
    bool canRedo() const;

    bool isInputMethodComposing() const;

    Qt::InputMethodHints imHints() const;
    void setIMHints(Qt::InputMethodHints hints);

    Q_INVOKABLE QString getText(int start, int end) const;

Q_SIGNALS:
    void textChanged();
    void cursorPositionChanged();
    void cursorRectangleChanged();
    void selectionStartChanged();
    void selectionEndChanged();
    void selectedTextChanged();
    void accepted();
    void acceptableInputChanged();
    void colorChanged(const QColor &color);
    void selectionColorChanged(const QColor &color);
    void selectedTextColorChanged(const QColor &color);
    void fontChanged(const QFont &font);
    void horizontalAlignmentChanged(HAlignment alignment);
    void verticalAlignmentChanged(VAlignment alignment);
    void wrapModeChanged();
    void readOnlyChanged(bool isReadOnly);
    void cursorVisibleChanged(bool isCursorVisible);
    void cursorDelegateChanged();
    void maximumLengthChanged(int maximumLength);
    void validatorChanged();
    void inputMaskChanged(const QString &inputMask);
    void echoModeChanged(EchoMode echoMode);
    void passwordCharacterChanged();
    void displayTextChanged();
    void activeFocusOnPressChanged(bool activeFocusOnPress);
    void autoScrollChanged(bool autoScroll);
    void selectByMouseChanged(bool selectByMouse);
    void mouseSelectionModeChanged(SelectionMode mode);
    void canPasteChanged();
    void canUndoChanged();
    void canRedoChanged();
    void inputMethodComposingChanged();
    void effectiveHorizontalAlignmentChanged();

protected:
    virtual void geometryChanged(const QRectF &newGeometry,
                                 const QRectF &oldGeometry);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent* ev);
    void inputMethodEvent(QInputMethodEvent *);
    void mouseUngrabEvent();
    bool event(QEvent *e);
    void focusInEvent(QFocusEvent *event);
    void timerEvent(QTimerEvent *event);
    virtual void itemChange(ItemChange, const ItemChangeData &);
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data);

public Q_SLOTS:
    void selectAll();
    void selectWord();
    void select(int start, int end);
    void deselect();
    bool isRightToLeft(int start, int end);
#ifndef QT_NO_CLIPBOARD
    void cut();
    void copy();
    void paste();
#endif
    void undo();
    void redo();
    void insert(int position, const QString &text);
    void remove(int start, int end);

private Q_SLOTS:
    void selectionChanged();
    void createCursor();
    void updateCursorRectangle();
    void q_canPasteChanged();

private:
    Q_DECLARE_PRIVATE(QQuickTextInput)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTextInput)
#ifndef QT_NO_VALIDATOR
QML_DECLARE_TYPE(QValidator)
QML_DECLARE_TYPE(QIntValidator)
QML_DECLARE_TYPE(QDoubleValidator)
QML_DECLARE_TYPE(QRegExpValidator)
#endif

QT_END_HEADER

#endif // QQUICKTEXTINPUT_P_H
