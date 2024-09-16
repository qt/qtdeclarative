// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import { name } from "info.mjs";

export function getName()
{
    return name;
}

export function func() {
    return "Hello World"
}

export {name as foo} from "info.mjs";
export * from "info.mjs";
