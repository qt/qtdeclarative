// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITFONT_H
#define QQSTYLEKITFONT_H

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

#include <QtQml/QtQml>
#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtGui/qfont.h>

QT_BEGIN_NAMESPACE

class QQStyleKitFont : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QFont system READ system WRITE setSystem NOTIFY systemChanged FINAL)
    Q_PROPERTY(QFont button READ button WRITE setButton NOTIFY buttonChanged FINAL)
    Q_PROPERTY(QFont checkBox READ checkBox WRITE setCheckBox NOTIFY checkBoxChanged FINAL)
    Q_PROPERTY(QFont comboBox READ comboBox WRITE setComboBox NOTIFY comboBoxChanged FINAL)
    Q_PROPERTY(QFont groupBox READ groupBox WRITE setGroupBox NOTIFY groupBoxChanged FINAL)
    Q_PROPERTY(QFont itemDelegate READ itemDelegate WRITE setItemDelegate NOTIFY itemDelegateChanged FINAL)
    Q_PROPERTY(QFont label READ label WRITE setLabel NOTIFY labelChanged FINAL)
    Q_PROPERTY(QFont radioButton READ radioButton WRITE setRadioButton NOTIFY radioButtonChanged FINAL)
    Q_PROPERTY(QFont spinBox READ spinBox WRITE setSpinBox NOTIFY spinBoxChanged FINAL)
    Q_PROPERTY(QFont switchControl READ switchControl WRITE setSwitchControl NOTIFY switchControlChanged FINAL)
    Q_PROPERTY(QFont tabBar READ tabBar WRITE setTabBar NOTIFY tabBarChanged FINAL)
    Q_PROPERTY(QFont textArea READ textArea WRITE setTextArea NOTIFY textAreaChanged FINAL)
    Q_PROPERTY(QFont textField READ textField WRITE setTextField NOTIFY textFieldChanged FINAL)
    Q_PROPERTY(QFont toolBar READ toolBar WRITE setToolBar NOTIFY toolBarChanged FINAL)

    QML_NAMED_ELEMENT(StyleFont)

public:
    QQStyleKitFont(QObject *parent = nullptr);
    QFont system() const;
    void setSystem(const QFont &font);

    QFont button() const;
    void setButton(const QFont &font);

    QFont checkBox() const;
    void setCheckBox(const QFont &font);

    QFont comboBox() const;
    void setComboBox(const QFont &font);

    QFont groupBox() const;
    void setGroupBox(const QFont &font);

    QFont itemDelegate() const;
    void setItemDelegate(const QFont &font);

    QFont label() const;
    void setLabel(const QFont &font);

    QFont radioButton() const;
    void setRadioButton(const QFont &font);

    QFont spinBox() const;
    void setSpinBox(const QFont &font);

    QFont switchControl() const;
    void setSwitchControl(const QFont &font);

    QFont tabBar() const;
    void setTabBar(const QFont &font);

    QFont textArea() const;
    void setTextArea(const QFont &font);

    QFont textField() const;
    void setTextField(const QFont &font);

    QFont toolBar() const;
    void setToolBar(const QFont &font);

    QQStyleKitFont *fallbackFont() const;
    void setFallbackFont(QQStyleKitFont *fallback);

signals:
    void systemChanged();
    void buttonChanged();
    void checkBoxChanged();
    void comboBoxChanged();
    void groupBoxChanged();
    void itemDelegateChanged();
    void labelChanged();
    void radioButtonChanged();
    void spinBoxChanged();
    void switchControlChanged();
    void tabBarChanged();
    void textAreaChanged();
    void textFieldChanged();
    void toolBarChanged();
    void fallbackFontChanged();

private:
    Q_DISABLE_COPY(QQStyleKitFont)

    QFont fontForScope(QQuickTheme::Scope scope) const;
    void setFontForScope(QQuickTheme::Scope scope, const QFont &font, void (QQStyleKitFont::*signal)());

    bool isSet(QQuickTheme::Scope scope) const { return (m_setMask & (1u << int(scope))) != 0; }
    void markSet(QQuickTheme::Scope scope) { m_setMask |= (1u << int(scope)); }

    static const int NScopes = QQuickTheme::Tumbler + 1;
    QFont m_fonts[NScopes];
    quint32 m_setMask = 0;

    QQStyleKitFont *m_fallback = nullptr;

    friend class QQStyleKitTheme;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITFONT_H
