// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITPALETTE_P_H
#define QQSTYLEKITPALETTE_P_H

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
#include <QtQuick/private/qquickpalette_p.h>

QT_BEGIN_NAMESPACE

class QQStyleKitPalette : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickPalette *system READ system NOTIFY systemChanged FINAL)
    Q_PROPERTY(QQuickPalette *checkBox READ checkBox NOTIFY checkBoxChanged FINAL)
    Q_PROPERTY(QQuickPalette *button READ button NOTIFY buttonChanged FINAL)
    Q_PROPERTY(QQuickPalette *comboBox READ comboBox NOTIFY comboBoxChanged FINAL)
    Q_PROPERTY(QQuickPalette *groupBox READ groupBox NOTIFY groupBoxChanged FINAL)
    Q_PROPERTY(QQuickPalette *itemDelegate READ itemDelegate NOTIFY itemDelegateChanged FINAL)
    Q_PROPERTY(QQuickPalette *label READ label NOTIFY labelChanged FINAL)
    Q_PROPERTY(QQuickPalette *radioButton READ radioButton NOTIFY radioButtonChanged FINAL)
    Q_PROPERTY(QQuickPalette *spinBox READ spinBox NOTIFY spinBoxChanged FINAL)
    Q_PROPERTY(QQuickPalette *switchControl READ switchControl NOTIFY switchControlChanged FINAL)
    Q_PROPERTY(QQuickPalette *tabBar READ tabBar NOTIFY tabBarChanged FINAL)
    Q_PROPERTY(QQuickPalette *textArea READ textArea NOTIFY textAreaChanged FINAL)
    Q_PROPERTY(QQuickPalette *textField READ textField NOTIFY textFieldChanged FINAL)
    Q_PROPERTY(QQuickPalette *toolBar READ toolBar NOTIFY toolBarChanged FINAL)

    QML_NAMED_ELEMENT(StyleKitPalette)

public:
    QQStyleKitPalette(QObject *parent = nullptr);

    QQuickPalette *system() const;
    QQuickPalette *checkBox() const;
    QQuickPalette *button() const;
    QQuickPalette *comboBox() const;
    QQuickPalette *groupBox() const;
    QQuickPalette *itemDelegate() const;
    QQuickPalette *label() const;
    QQuickPalette *radioButton() const;
    QQuickPalette *spinBox() const;
    QQuickPalette *switchControl() const;
    QQuickPalette *tabBar() const;
    QQuickPalette *textArea() const;
    QQuickPalette *textField() const;
    QQuickPalette *toolBar() const;

    QQStyleKitPalette *fallbackPalette() const;
    void setFallbackPalette(QQStyleKitPalette *fallback);

signals:
    void systemChanged();
    void checkBoxChanged();
    void buttonChanged();
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
    void fallbackPaletteChanged();

private:
    Q_DISABLE_COPY(QQStyleKitPalette)
    /*
        The following properties are lazy-created, since it's unlikely that a style
        sets them all. And since a QQuickPalette is not a QObject, we use std::unique_ptr
        for memory management. Since each palette is logically independent of this class,
        we make them mutable so that the getter functions can be const.
    */
    mutable std::unique_ptr<QQuickPalette> m_system;
    mutable std::unique_ptr<QQuickPalette> m_checkBox;
    mutable std::unique_ptr<QQuickPalette> m_button;
    mutable std::unique_ptr<QQuickPalette> m_comboBox;
    mutable std::unique_ptr<QQuickPalette> m_groupBox;
    mutable std::unique_ptr<QQuickPalette> m_itemDelegate;
    mutable std::unique_ptr<QQuickPalette> m_label;
    mutable std::unique_ptr<QQuickPalette> m_radioButton;
    mutable std::unique_ptr<QQuickPalette> m_spinBox;
    mutable std::unique_ptr<QQuickPalette> m_switchControl;
    mutable std::unique_ptr<QQuickPalette> m_tabBar;
    mutable std::unique_ptr<QQuickPalette> m_textArea;
    mutable std::unique_ptr<QQuickPalette> m_textField;
    mutable std::unique_ptr<QQuickPalette> m_toolBar;

    QQStyleKitPalette *m_fallbackPalette = nullptr;

    quint32 m_setMask = 0;
    bool isSet(QQuickTheme::Scope scope) const { return (m_setMask & (1u << int(scope))) != 0; }
    void markSet(QQuickTheme::Scope scope) { m_setMask |= (1u << int(scope)); }

    friend class QQStyleKitTheme;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITPALETTE_P_H
