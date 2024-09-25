// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "statecontroller.h"

StateController::StateController(QObject *parent)
    : QObject{parent}
{}

StateController *StateController::instance()
{
    if (!s_instance)
        s_instance = new StateController;

    return s_instance;
}

void StateController::setDirty(bool dirty)
{
    switch (currentState()) {
    case InitState:
        if (dirty)
            setCurrentState(NewState);
        break;

    case NewState:
        if (!dirty)
            setCurrentState(InitState);
        break;

    case OpenState:
        if (dirty)
            setCurrentState(DirtyState);
        break;

    case DirtyState:
        if (!dirty)
            setCurrentState(OpenState);
        break;

    default:
        break;
    }
}

void StateController::changesSaved(const QString &filePath)
{
    switch (currentState()) {
    case NewState:
    case DirtyState:
        if (!filePath.isNull())
            setFilePath(filePath);
        setCurrentState(OpenState);
        break;

    default:
        break;
    }
}

void StateController::fileLoaded(const QString &filePath)
{
    switch (currentState()) {
    case InitState:
        setFilePath(filePath);
        setCurrentState(OpenState);
        break;

    case OpenState:
        setFilePath(filePath);
        emit stateChanged(OpenState, OpenState);
        break;

    default:
        break;
    }
}

void StateController::fileClosed()
{
    switch (currentState()) {
    case OpenState:
        setFilePath();
        setCurrentState(InitState);
        break;

    default:
        break;
    }
}

void StateController::setCurrentState(State state)
{
    const State old = currentState();
    if (old == state)
        return;

    m_currentState = state;
    emit stateChanged(old, state);
}
