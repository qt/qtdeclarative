:: Copyright (C) 2024 The Qt Company Ltd.
:: SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

@echo off
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2023 Googlefonts\nSPDX-License-Identifier: Apache-2.0" cloud-svgrepo-com.svg ..\Cloud_generated.qml
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2023 Googlefonts\nSPDX-License-Identifier: Apache-2.0" cloud-with-lightning-svgrepo-com.svg ..\CloudWithLightning_generated.qml
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2023 Googlefonts\nSPDX-License-Identifier: Apache-2.0" cloud-with-lightning-and-rain-svgrepo-com.svg ..\CloudWithLightningAndRain_generated.qml
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2023 Googlefonts\nSPDX-License-Identifier: Apache-2.0" cloud-with-rain-svgrepo-com.svg ..\CloudWithRain_generated.qml
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2023 Googlefonts\nSPDX-License-Identifier: Apache-2.0" cloud-with-snow-svgrepo-com.svg ..\CloudWithSnow_generated.qml
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2024 The Qt Company Ltd.\nCopyright (C) 2010 Kolja21\nSPDX-License-Identifier: CC-BY-3.0" Europe.svg ..\Europe_generated.qml
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2023 Framework7\nSPDX-License-Identifier: MIT" gear-alt-stroke.svg ..\Gear_generated.qml
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2023 Googlefonts\nSPDX-License-Identifier: Apache-2.0" sun-svgrepo-com.svg ..\Sun_generated.qml
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2023 Googlefonts\nSPDX-License-Identifier: Apache-2.0" sun-behind-large-cloud-svgrepo-com.svg ..\SunBehindLargeCloud_generated.qml
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2023 Googlefonts\nSPDX-License-Identifier: Apache-2.0" sun-behind-rain-cloud-svgrepo-com.svg ..\SunBehindRainCloud_generated.qml
svgtoqml --optimize-paths -t DemoShape --copyright-statement "Copyright (C) 2023 Googlefonts\nSPDX-License-Identifier: Apache-2.0" sun-behind-small-cloud-svgrepo-com.svg ..\SunBehindSmallCloud_generated.qml
