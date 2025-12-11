// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITCONTROLS_P_H
#define QQSTYLEKITCONTROLS_P_H

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

#include "qqstylekitreader_p.h"

QT_BEGIN_NAMESPACE

class QQStyleKitStyle;
class QQStyleKitControl;
class QQStyleKitCustomControl;

class QQStyleKitControls : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QQStyleKitControl *abstractButton READ abstractButton WRITE set_abstractButton NOTIFY abstractButtonChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *applicationWindow READ applicationWindow WRITE set_applicationWindow NOTIFY applicationWindowChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *control READ control WRITE set_control NOTIFY controlChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *button READ button WRITE set_button NOTIFY buttonChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *checkBox READ checkBox WRITE set_checkBox NOTIFY checkBoxChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *comboBox READ comboBox WRITE set_comboBox NOTIFY comboBoxChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *flatButton READ flatButton WRITE set_flatButton NOTIFY flatButtonChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *progressBar READ progressBar WRITE set_progressBar NOTIFY progressBarChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *scrollBar READ scrollBar WRITE set_scrollBar NOTIFY scrollBarChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *scrollIndicator READ scrollIndicator WRITE set_scrollIndicator NOTIFY scrollIndicatorChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *scrollView READ scrollView WRITE set_scrollView NOTIFY scrollViewChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *slider READ slider WRITE set_slider NOTIFY sliderChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *spinBox READ spinBox WRITE set_spinBox NOTIFY spinBoxChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *tabBar READ tabBar WRITE set_tabBar NOTIFY tabBarChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *tabButton READ tabButton WRITE set_tabButton NOTIFY tabButtonChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *textField READ textField WRITE set_textField NOTIFY textFieldChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *textInput READ textInput WRITE set_textInput NOTIFY textInputChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *toolBar READ toolBar WRITE set_toolBar NOTIFY toolBarChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *toolButton READ toolButton WRITE set_toolButton NOTIFY toolButtonChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *toolSeparator READ toolSeparator WRITE set_toolSeparator NOTIFY toolSeparatorChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *switchControl READ switchControl WRITE set_switchControl NOTIFY switchControlChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *radioButton READ radioButton WRITE set_radioButton NOTIFY radioButtonChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *itemDelegate READ itemDelegate WRITE set_itemDelegate NOTIFY itemDelegateChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *popup READ popup WRITE set_popup NOTIFY popupChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *pane READ pane WRITE set_pane NOTIFY paneChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *page READ page WRITE set_page NOTIFY pageChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *frame READ frame WRITE set_frame NOTIFY frameChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *label READ label WRITE set_label NOTIFY labelChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *groupBox READ groupBox WRITE set_groupBox NOTIFY groupBoxChanged FINAL)
    Q_PROPERTY(QQStyleKitControl *textArea READ textArea WRITE set_textArea NOTIFY textAreaChanged FINAL)
    QML_UNCREATABLE("This component is abstract, and cannot be instantiated")
    QML_NAMED_ELEMENT(StyleKitControls)

    Q_PROPERTY(QQmlListProperty<QObject> data READ data NOTIFY dataChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    QQStyleKitControls(QObject *parent = nullptr);

#define IMPLEMENT_ACCESSORS(NAME) \
    QQStyleKitControl *NAME() const; \
    void set_ ## NAME(QQStyleKitControl *control);

    IMPLEMENT_ACCESSORS(applicationWindow)
    IMPLEMENT_ACCESSORS(abstractButton)
    IMPLEMENT_ACCESSORS(control)
    IMPLEMENT_ACCESSORS(button)
    IMPLEMENT_ACCESSORS(checkBox)
    IMPLEMENT_ACCESSORS(comboBox)
    IMPLEMENT_ACCESSORS(flatButton)
    IMPLEMENT_ACCESSORS(progressBar)
    IMPLEMENT_ACCESSORS(scrollBar)
    IMPLEMENT_ACCESSORS(scrollIndicator)
    IMPLEMENT_ACCESSORS(scrollView)
    IMPLEMENT_ACCESSORS(slider)
    IMPLEMENT_ACCESSORS(spinBox)
    IMPLEMENT_ACCESSORS(tabBar)
    IMPLEMENT_ACCESSORS(tabButton)
    IMPLEMENT_ACCESSORS(textField)
    IMPLEMENT_ACCESSORS(textInput)
    IMPLEMENT_ACCESSORS(toolBar)
    IMPLEMENT_ACCESSORS(toolButton)
    IMPLEMENT_ACCESSORS(toolSeparator)
    IMPLEMENT_ACCESSORS(switchControl)
    IMPLEMENT_ACCESSORS(radioButton)
    IMPLEMENT_ACCESSORS(itemDelegate)
    IMPLEMENT_ACCESSORS(popup)
    IMPLEMENT_ACCESSORS(pane)
    IMPLEMENT_ACCESSORS(page)
    IMPLEMENT_ACCESSORS(frame)
    IMPLEMENT_ACCESSORS(label)
    IMPLEMENT_ACCESSORS(groupBox)
    IMPLEMENT_ACCESSORS(textArea)

#undef IMPLEMENT_ACCESSORS

    Q_INVOKABLE QQStyleKitControl *getControl(QQStyleKitExtendableControlType controlType) const;

    QList<QQStyleKitVariation *> variations() const;

    QQmlListProperty<QObject> data();
    const QList<QObject *> children() const;

signals:
    void dataChanged();
    void applicationWindowChanged();
    void abstractButtonChanged();
    void controlChanged();
    void buttonChanged();
    void checkBoxChanged();
    void comboBoxChanged();
    void flatButtonChanged();
    void progressBarChanged();
    void scrollBarChanged();
    void scrollIndicatorChanged();
    void scrollViewChanged();
    void sliderChanged();
    void spinBoxChanged();
    void tabBarChanged();
    void tabButtonChanged();
    void textFieldChanged();
    void textInputChanged();
    void toolBarChanged();
    void toolButtonChanged();
    void toolSeparatorChanged();
    void switchControlChanged();
    void radioButtonChanged();
    void itemDelegateChanged();
    void popupChanged();
    void paneChanged();
    void pageChanged();
    void frameChanged();
    void labelChanged();
    void groupBoxChanged();
    void textAreaChanged();

protected:
    void classBegin() override {}
    void componentComplete() override;

private:
    Q_DISABLE_COPY(QQStyleKitControls)

    QList<QObject *> m_data;
    QHash<QQStyleKitExtendableControlType, QQStyleKitControl *> m_controls;
    QHash<PropertyPathId_t, QQSK::State> m_writtenPropertyPaths;

    friend class QQStyleKitPropertyResolver;
    friend class QQStyleKitControl;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITCONTROLS_P_H
