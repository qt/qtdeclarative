/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

ListModel {
    id: stocks
    // Data from : http://en.wikipedia.org/wiki/NASDAQ-100
    ListElement {name: "Activision Blizzard"; stockId: "ATVI"}
    ListElement {name: "Adobe Systems Incorporated"; stockId: "ADBE"}
    ListElement {name: "Akamai Technologies, Inc"; stockId: "AKAM"}
    ListElement {name: "Alexion Pharmaceuticals"; stockId: "ALXN"}
    ListElement {name: "Altera Corporation"; stockId: "ALTR"}
    ListElement {name: "Amazon.com, Inc."; stockId: "AMZN"}
    ListElement {name: "Amgen Inc."; stockId: "AMGN"}
    ListElement {name: "Apollo Group, Inc."; stockId: "APOL"}
    ListElement {name: "Apple Inc."; stockId: "AAPL"}
    ListElement {name: "Applied Materials, Inc."; stockId: "AMAT"}
    ListElement {name: "Autodesk, Inc."; stockId: "ADSK"}
    ListElement {name: "Automatic Data Processing, Inc."; stockId: "ADP"}
    ListElement {name: "Baidu.com, Inc."; stockId: "BIDU"}
    ListElement {name: "Bed Bath & Beyond Inc."; stockId: "BBBY"}
    ListElement {name: "Biogen Idec, Inc"; stockId: "BIIB"}
    ListElement {name: "BMC Software, Inc."; stockId: "BMC"}
    ListElement {name: "Broadcom Corporation"; stockId: "BRCM"}
    ListElement {name: "C. H. Robinson Worldwide, Inc."; stockId: "CHRW"}
    ListElement {name: "CA, Inc."; stockId: "CA"}
    ListElement {name: "Celgene Corporation"; stockId: "CELG"}
    ListElement {name: "Cephalon, Inc."; stockId: "CEPH"}
    ListElement {name: "Cerner Corporation"; stockId: "CERN"}
    ListElement {name: "Check Point Software Technologies Ltd."; stockId: "CHKP"}
    ListElement {name: "Cisco Systems, Inc."; stockId: "CSCO"}
    ListElement {name: "Citrix Systems, Inc."; stockId: "CTXS"}
    ListElement {name: "Cognizant Technology Solutions Corporation"; stockId: "CTSH"}
    ListElement {name: "Comcast Corporation"; stockId: "CMCSA"}
    ListElement {name: "Costco Wholesale Corporation"; stockId: "COST"}
    ListElement {name: "Ctrip.com International, Ltd."; stockId: "CTRP"}
    ListElement {name: "Dell Inc."; stockId: "DELL"}
    ListElement {name: "DENTSPLY International Inc."; stockId: "XRAY"}
    ListElement {name: "DirecTV"; stockId: "DTV"}
    ListElement {name: "Dollar Tree, Inc."; stockId: "DLTR"}
    ListElement {name: "eBay Inc."; stockId: "EBAY"}
    ListElement {name: "Electronic Arts Inc."; stockId: "ERTS"}
    ListElement {name: "Expedia, Inc."; stockId: "EXPE"}
    ListElement {name: "Expeditors International of Washington, Inc."; stockId: "EXPD"}
    ListElement {name: "Express Scripts, Inc."; stockId: "ESRX"}
    ListElement {name: "F5 Networks, Inc."; stockId: "FFIV"}
    ListElement {name: "Fastenal Company"; stockId: "FAST"}
    ListElement {name: "First Solar, Inc."; stockId: "FSLR"}
    ListElement {name: "Fiserv, Inc."; stockId: "FISV"}
    ListElement {name: "Flextronics International Ltd."; stockId: "FLEX"}
    ListElement {name: "FLIR Systems, Inc."; stockId: "FLIR"}
    ListElement {name: "Garmin Ltd."; stockId: "GRMN"}
    ListElement {name: "Gilead Sciences, Inc."; stockId: "GILD"}
    ListElement {name: "Google Inc."; stockId: "GOOG"}
    ListElement {name: "Green Mountain Coffee Roasters, Inc."; stockId: "GMCR"}
    ListElement {name: "Henry Schein, Inc."; stockId: "HSIC"}
    ListElement {name: "Illumina, Inc."; stockId: "ILMN"}
    ListElement {name: "Infosys Technologies"; stockId: "INFY"}
    ListElement {name: "Intel Corporation"; stockId: "INTC"}
    ListElement {name: "Intuit, Inc."; stockId: "INTU"}
    ListElement {name: "Intuitive Surgical Inc."; stockId: "ISRG"}
    ListElement {name: "Joy Global Inc."; stockId: "JOYG"}
    ListElement {name: "KLA Tencor Corporation"; stockId: "KLAC"}
    ListElement {name: "Lam Research Corporation"; stockId: "LRCX"}
    ListElement {name: "Liberty Media Corporation, Interactive Series A"; stockId: "LINTA"}
    ListElement {name: "Life Technologies Corporation"; stockId: "LIFE"}
    ListElement {name: "Linear Technology Corporation"; stockId: "LLTC"}
    ListElement {name: "Marvell Technology Group, Ltd."; stockId: "MRVL"}
    ListElement {name: "Mattel, Inc."; stockId: "MAT"}
    ListElement {name: "Maxim Integrated Products"; stockId: "MXIM"}
    ListElement {name: "Microchip Technology Incorporated"; stockId: "MCHP"}
    ListElement {name: "Micron Technology, Inc."; stockId: "MU"}
    ListElement {name: "Microsoft Corporation"; stockId: "MSFT"}
    ListElement {name: "Mylan, Inc."; stockId: "MYL"}
    ListElement {name: "NetApp, Inc."; stockId: "NTAP"}
    ListElement {name: "Netflix, Inc."; stockId: "NFLX"}
    ListElement {name: "News Corporation, Ltd."; stockId: "NWSA"}
    ListElement {name: "NII Holdings, Inc."; stockId: "NIHD"}
    ListElement {name: "NVIDIA Corporation"; stockId: "NVDA"}
    ListElement {name: "O'Reilly Automotive, Inc."; stockId: "ORLY"}
    ListElement {name: "Oracle Corporation"; stockId: "ORCL"}
    ListElement {name: "PACCAR Inc."; stockId: "PCAR"}
    ListElement {name: "Paychex, Inc."; stockId: "PAYX"}
    ListElement {name: "Priceline.com, Incorporated"; stockId: "PCLN"}
    ListElement {name: "Qiagen N.V."; stockId: "QGEN"}
    ListElement {name: "QUALCOMM Incorporated"; stockId: "QCOM"}
    ListElement {name: "Research in Motion Limited"; stockId: "RIMM"}
    ListElement {name: "Ross Stores Inc."; stockId: "ROST"}
    ListElement {name: "SanDisk Corporation"; stockId: "SNDK"}
    ListElement {name: "Seagate Technology Holdings"; stockId: "STX"}
    ListElement {name: "Sears Holdings Corporation"; stockId: "SHLD"}
    ListElement {name: "Sigma-Aldrich Corporation"; stockId: "SIAL"}
    ListElement {name: "Staples Inc."; stockId: "SPLS"}
    ListElement {name: "Starbucks Corporation"; stockId: "SBUX"}
    ListElement {name: "Stericycle, Inc"; stockId: "SRCL"}
    ListElement {name: "Symantec Corporation"; stockId: "SYMC"}
    ListElement {name: "Teva Pharmaceutical Industries Ltd."; stockId: "TEVA"}
    ListElement {name: "Urban Outfitters, Inc."; stockId: "URBN"}
    ListElement {name: "VeriSign, Inc."; stockId: "VRSN"}
    ListElement {name: "Vertex Pharmaceuticals"; stockId: "VRTX"}
    ListElement {name: "Virgin Media, Inc."; stockId: "VMED"}
    ListElement {name: "Vodafone Group, plc."; stockId: "VOD"}
    ListElement {name: "Warner Chilcott, Ltd."; stockId: "WCRX"}
    ListElement {name: "Whole Foods Market, Inc."; stockId: "WFM"}
    ListElement {name: "Wynn Resorts Ltd."; stockId: "WYNN"}
    ListElement {name: "Xilinx, Inc."; stockId: "XLNX"}
    ListElement {name: "Yahoo! Inc."; stockId: "YHOO"}
}
