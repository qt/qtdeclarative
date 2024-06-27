// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "cppsettings.h"

#include <QCoreApplication>

CppSettings::CppSettings(QObject *parent) :
    QObject(parent),
    mSettings("QtProject", "menus")
{
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, dontUseNativeMenuBar());
}

bool CppSettings::dontUseNativeMenuBar() const
{
    return mSettings.value("dontUseNativeMenuBar").toBool();
}

void CppSettings::setDontUseNativeMenuBar(bool dontUseNativeMenuBar)
{
    const bool oldValue = this->dontUseNativeMenuBar();
    if (dontUseNativeMenuBar == oldValue)
        return;

    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, dontUseNativeMenuBar);
    mSettings.setValue("dontUseNativeMenuBar", dontUseNativeMenuBar);
    emit dontUseNativeMenuBarChanged();
}

int CppSettings::popupType() const
{
    return mSettings.value("popupType").toInt();
}

void CppSettings::setPopupType(int newPopupType)
{
    const int oldValue = popupType();
    if (oldValue == newPopupType)
        return;
    mSettings.setValue("popupType", newPopupType);
    emit popupTypeChanged();
}

bool CppSettings::dontUseNativeMenuWindows() const
{
    return mSettings.value("dontUseNativeMenuWindows").toBool();
}

void CppSettings::setDontUseNativeMenuWindows(bool dontUseNativeMenuWindows)
{
    const bool oldValue = this->dontUseNativeMenuWindows();
    if (dontUseNativeMenuWindows == oldValue)
        return;

    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows, dontUseNativeMenuWindows);
    mSettings.setValue("dontUseNativeMenuWindows", dontUseNativeMenuWindows);
    emit dontUseNativeMenuWindowsChanged();
}
