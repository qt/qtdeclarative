import QtQuick
import Qt.labs.StyleKit

/*
    Every color and number property in a control is assigned a value on one or more of the
    following levels, from least to most specific:

    1. fallbackStyle.control          4. control           7. light.control
    2. fallbackStyle.abstractButton   5. abstractButton    8. light.abstractButton
    3. fallbackStyle.button           6. button            9. light.button

    Each property has a unique id, and is assigned on every level from 1 up to a property
    specific max level. The value assigned on a level is "id * 10 + level". For opacity
    and scale it's "id / 1000 + level / 10000" (+ 1 for scale). For colors, the red
    component is a unique color id * 5, and the green component is level * 25.
    A property is always assigned on at least the same levels as the properties it
    falls back to (e.g. leftPadding -> padding, and handle.first -> handle).
*/
Style {
    // Force light theme to be used, so that the test is not dependent on the system theme
    themeName: "Light"

    fallbackStyle: Style {
        control { // level 1
            spacing: 11 // final assignment
            padding: 21
            leftPadding: 31
            rightPadding: 41
            topPadding: 51
            bottomPadding: 61
            background {
                width: 71
                height: 81
                minimumWidth: 91
                minimumHeight: 101 // final assignment
                margins: 111
                leftMargin: 121
                rightMargin: 131
                topMargin: 141
                bottomMargin: 151
                radius: 161
                topLeftRadius: 171
                topRightRadius: 181
                bottomLeftRadius: 191
                bottomRightRadius: 201
                rotation: 211
                scale: 1.0221
                opacity: 0.0231
                color: "#051925"
                border {
                    width: 251
                    color: "#0a194a"
                }
                shadow {
                    color: "#0f196f"
                    opacity: 0.0281 // final assignment
                    scale: 1.0291
                    verticalOffset: 301
                    horizontalOffset: 311
                    blur: 321
                }
                image {
                    color: "#141994"
                }
            }
            handle {
                width: 341
                height: 351
                minimumWidth: 361
                minimumHeight: 371 // final assignment
                margins: 381
                leftMargin: 391
                rightMargin: 401
                topMargin: 411
                bottomMargin: 421
                radius: 431
                topLeftRadius: 441
                topRightRadius: 451
                bottomLeftRadius: 461
                bottomRightRadius: 471
                rotation: 481
                scale: 1.0491
                opacity: 0.0501
                color: "#1919b9"
                border {
                    width: 521
                    color: "#1e19de"
                }
                shadow {
                    color: "#231903"
                    opacity: 0.0551 // final assignment
                    scale: 1.0561
                    verticalOffset: 571
                    horizontalOffset: 581
                    blur: 591
                }
                image {
                    color: "#281928"
                }
                first {
                    width: 611
                    height: 621
                    minimumWidth: 631
                    minimumHeight: 641 // final assignment
                    margins: 651
                    leftMargin: 661
                    rightMargin: 671
                    topMargin: 681
                    bottomMargin: 691
                    radius: 701
                    topLeftRadius: 711
                    topRightRadius: 721
                    bottomLeftRadius: 731
                    bottomRightRadius: 741
                    rotation: 751
                    scale: 1.0761
                    opacity: 0.0771
                    color: "#2d194d"
                    border {
                        width: 791
                        color: "#321972"
                    }
                    shadow {
                        color: "#371997"
                        opacity: 0.0821 // final assignment
                        scale: 1.0831
                        verticalOffset: 841
                        horizontalOffset: 851
                        blur: 861
                    }
                    image {
                        color: "#3c19bc"
                    }
                }
                second {
                    width: 881
                    height: 891
                    minimumWidth: 901
                    minimumHeight: 911 // final assignment
                    margins: 921
                    leftMargin: 931
                    rightMargin: 941
                    topMargin: 951
                    bottomMargin: 961
                    radius: 971
                    topLeftRadius: 981
                    topRightRadius: 991
                    bottomLeftRadius: 1001
                    bottomRightRadius: 1011
                    rotation: 1021
                    scale: 1.1031
                    opacity: 0.1041
                    color: "#4119e1"
                    border {
                        width: 1061
                        color: "#461906"
                    }
                    shadow {
                        color: "#4b192b"
                        opacity: 0.1091 // final assignment
                        scale: 1.1101
                        verticalOffset: 1111
                        horizontalOffset: 1121
                        blur: 1131
                    }
                    image {
                        color: "#501950"
                    }
                }
            }
            indicator {
                width: 1151
                height: 1161
                minimumWidth: 1171
                minimumHeight: 1181 // final assignment
                margins: 1191
                leftMargin: 1201
                rightMargin: 1211
                topMargin: 1221
                bottomMargin: 1231
                radius: 1241
                topLeftRadius: 1251
                topRightRadius: 1261
                bottomLeftRadius: 1271
                bottomRightRadius: 1281
                rotation: 1291
                scale: 1.1301
                opacity: 0.1311
                color: "#551975"
                border {
                    width: 1331
                    color: "#5a199a"
                }
                shadow {
                    color: "#5f19bf"
                    opacity: 0.1361 // final assignment
                    scale: 1.1371
                    verticalOffset: 1381
                    horizontalOffset: 1391
                    blur: 1401
                }
                image {
                    color: "#6419e4"
                }
                foreground {
                    width: 1421
                    height: 1431
                    minimumWidth: 1441
                    minimumHeight: 1451 // final assignment
                    margins: 1461
                    leftMargin: 1471
                    rightMargin: 1481
                    topMargin: 1491
                    bottomMargin: 1501
                    radius: 1511
                    topLeftRadius: 1521
                    topRightRadius: 1531
                    bottomLeftRadius: 1541
                    bottomRightRadius: 1551
                    rotation: 1561
                    scale: 1.1571
                    opacity: 0.1581
                    color: "#691909"
                    border {
                        width: 1601
                        color: "#6e192e"
                    }
                    shadow {
                        color: "#731953"
                        opacity: 0.1631 // final assignment
                        scale: 1.1641
                        verticalOffset: 1651
                        horizontalOffset: 1661
                        blur: 1671
                    }
                    image {
                        color: "#781978"
                    }
                }
                first {
                    width: 1691
                    height: 1701
                    minimumWidth: 1711
                    minimumHeight: 1721 // final assignment
                    margins: 1731
                    leftMargin: 1741
                    rightMargin: 1751
                    topMargin: 1761
                    bottomMargin: 1771
                    radius: 1781
                    topLeftRadius: 1791
                    topRightRadius: 1801
                    bottomLeftRadius: 1811
                    bottomRightRadius: 1821
                    rotation: 1831
                    scale: 1.1841
                    opacity: 0.1851
                    color: "#7d199d"
                    border {
                        width: 1871
                        color: "#8219c2"
                    }
                    shadow {
                        color: "#8719e7"
                        opacity: 0.1901 // final assignment
                        scale: 1.1911
                        verticalOffset: 1921
                        horizontalOffset: 1931
                        blur: 1941
                    }
                    image {
                        color: "#8c190c"
                    }
                    foreground {
                        width: 1961
                        height: 1971
                        minimumWidth: 1981
                        minimumHeight: 1991 // final assignment
                        margins: 2001
                        leftMargin: 2011
                        rightMargin: 2021
                        topMargin: 2031
                        bottomMargin: 2041
                        radius: 2051
                        topLeftRadius: 2061
                        topRightRadius: 2071
                        bottomLeftRadius: 2081
                        bottomRightRadius: 2091
                        rotation: 2101
                        scale: 1.2111
                        opacity: 0.2121
                        color: "#911931"
                        border {
                            width: 2141
                            color: "#961956"
                        }
                        shadow {
                            color: "#9b197b"
                            opacity: 0.2171 // final assignment
                            scale: 1.2181
                            verticalOffset: 2191
                            horizontalOffset: 2201
                            blur: 2211
                        }
                        image {
                            color: "#a019a0"
                        }
                    }
                }
                second {
                    width: 2231
                    height: 2241
                    minimumWidth: 2251
                    minimumHeight: 2261 // final assignment
                    margins: 2271
                    leftMargin: 2281
                    rightMargin: 2291
                    topMargin: 2301
                    bottomMargin: 2311
                    radius: 2321
                    topLeftRadius: 2331
                    topRightRadius: 2341
                    bottomLeftRadius: 2351
                    bottomRightRadius: 2361
                    rotation: 2371
                    scale: 1.2381
                    opacity: 0.2391
                    color: "#a519c5"
                    border {
                        width: 2411
                        color: "#aa19ea"
                    }
                    shadow {
                        color: "#af190f"
                        opacity: 0.2441 // final assignment
                        scale: 1.2451
                        verticalOffset: 2461
                        horizontalOffset: 2471
                        blur: 2481
                    }
                    image {
                        color: "#b41934"
                    }
                    foreground {
                        width: 2501
                        height: 2511
                        minimumWidth: 2521
                        minimumHeight: 2531 // final assignment
                        margins: 2541
                        leftMargin: 2551
                        rightMargin: 2561
                        topMargin: 2571
                        bottomMargin: 2581
                        radius: 2591
                        topLeftRadius: 2601
                        topRightRadius: 2611
                        bottomLeftRadius: 2621
                        bottomRightRadius: 2631
                        rotation: 2641
                        scale: 1.2651
                        opacity: 0.2661
                        color: "#b91959"
                        border {
                            width: 2681
                            color: "#be197e"
                        }
                        shadow {
                            color: "#c319a3"
                            opacity: 0.2711 // final assignment
                            scale: 1.2721
                            verticalOffset: 2731
                            horizontalOffset: 2741
                            blur: 2751
                        }
                        image {
                            color: "#c819c8"
                        }
                    }
                }
            }
            text {
                color: "#cd19ed"
                pointSize: 2781
                padding: 2791
                leftPadding: 2801
                rightPadding: 2811
                topPadding: 2821
                bottomPadding: 2831
                first {
                    color: "#d21912"
                    pointSize: 2851
                    padding: 2861
                    leftPadding: 2871
                    rightPadding: 2881
                    topPadding: 2891
                    bottomPadding: 2901
                }
                second {
                    color: "#d71937"
                    pointSize: 2921
                    padding: 2931
                    leftPadding: 2941
                    rightPadding: 2951
                    topPadding: 2961
                    bottomPadding: 2971
                }
            }
        }

        abstractButton { // level 2
            padding: 22 // final assignment
            leftPadding: 32
            rightPadding: 42
            topPadding: 52
            bottomPadding: 62
            background {
                width: 72
                height: 82
                minimumWidth: 92
                margins: 112 // final assignment
                leftMargin: 122
                rightMargin: 132
                topMargin: 142
                bottomMargin: 152
                radius: 162
                topLeftRadius: 172
                topRightRadius: 182
                bottomLeftRadius: 192
                bottomRightRadius: 202
                rotation: 212
                scale: 1.0222
                opacity: 0.0232
                color: "#053225"
                border {
                    width: 252
                    color: "#0a324a"
                }
                shadow {
                    color: "#0f326f"
                    scale: 1.0292 // final assignment
                    verticalOffset: 302
                    horizontalOffset: 312
                    blur: 322
                }
                image {
                    color: "#143294"
                }
            }
            handle {
                width: 342
                height: 352
                minimumWidth: 362
                margins: 382 // final assignment
                leftMargin: 392
                rightMargin: 402
                topMargin: 412
                bottomMargin: 422
                radius: 432
                topLeftRadius: 442
                topRightRadius: 452
                bottomLeftRadius: 462
                bottomRightRadius: 472
                rotation: 482
                scale: 1.0492
                opacity: 0.0502
                color: "#1932b9"
                border {
                    width: 522
                    color: "#1e32de"
                }
                shadow {
                    color: "#233203"
                    scale: 1.0562 // final assignment
                    verticalOffset: 572
                    horizontalOffset: 582
                    blur: 592
                }
                image {
                    color: "#283228"
                }
                first {
                    width: 612
                    height: 622
                    minimumWidth: 632
                    margins: 652 // final assignment
                    leftMargin: 662
                    rightMargin: 672
                    topMargin: 682
                    bottomMargin: 692
                    radius: 702
                    topLeftRadius: 712
                    topRightRadius: 722
                    bottomLeftRadius: 732
                    bottomRightRadius: 742
                    rotation: 752
                    scale: 1.0762
                    opacity: 0.0772
                    color: "#2d324d"
                    border {
                        width: 792
                        color: "#323272"
                    }
                    shadow {
                        color: "#373297"
                        scale: 1.0832 // final assignment
                        verticalOffset: 842
                        horizontalOffset: 852
                        blur: 862
                    }
                    image {
                        color: "#3c32bc"
                    }
                }
                second {
                    width: 882
                    height: 892
                    minimumWidth: 902
                    margins: 922 // final assignment
                    leftMargin: 932
                    rightMargin: 942
                    topMargin: 952
                    bottomMargin: 962
                    radius: 972
                    topLeftRadius: 982
                    topRightRadius: 992
                    bottomLeftRadius: 1002
                    bottomRightRadius: 1012
                    rotation: 1022
                    scale: 1.1032
                    opacity: 0.1042
                    color: "#4132e1"
                    border {
                        width: 1062
                        color: "#463206"
                    }
                    shadow {
                        color: "#4b322b"
                        scale: 1.1102 // final assignment
                        verticalOffset: 1112
                        horizontalOffset: 1122
                        blur: 1132
                    }
                    image {
                        color: "#503250"
                    }
                }
            }
            indicator {
                width: 1152
                height: 1162
                minimumWidth: 1172
                margins: 1192 // final assignment
                leftMargin: 1202
                rightMargin: 1212
                topMargin: 1222
                bottomMargin: 1232
                radius: 1242
                topLeftRadius: 1252
                topRightRadius: 1262
                bottomLeftRadius: 1272
                bottomRightRadius: 1282
                rotation: 1292
                scale: 1.1302
                opacity: 0.1312
                color: "#553275"
                border {
                    width: 1332
                    color: "#5a329a"
                }
                shadow {
                    color: "#5f32bf"
                    scale: 1.1372 // final assignment
                    verticalOffset: 1382
                    horizontalOffset: 1392
                    blur: 1402
                }
                image {
                    color: "#6432e4"
                }
                foreground {
                    width: 1422
                    height: 1432
                    minimumWidth: 1442
                    margins: 1462 // final assignment
                    leftMargin: 1472
                    rightMargin: 1482
                    topMargin: 1492
                    bottomMargin: 1502
                    radius: 1512
                    topLeftRadius: 1522
                    topRightRadius: 1532
                    bottomLeftRadius: 1542
                    bottomRightRadius: 1552
                    rotation: 1562
                    scale: 1.1572
                    opacity: 0.1582
                    color: "#693209"
                    border {
                        width: 1602
                        color: "#6e322e"
                    }
                    shadow {
                        color: "#733253"
                        scale: 1.1642 // final assignment
                        verticalOffset: 1652
                        horizontalOffset: 1662
                        blur: 1672
                    }
                    image {
                        color: "#783278"
                    }
                }
                first {
                    width: 1692
                    height: 1702
                    minimumWidth: 1712
                    margins: 1732 // final assignment
                    leftMargin: 1742
                    rightMargin: 1752
                    topMargin: 1762
                    bottomMargin: 1772
                    radius: 1782
                    topLeftRadius: 1792
                    topRightRadius: 1802
                    bottomLeftRadius: 1812
                    bottomRightRadius: 1822
                    rotation: 1832
                    scale: 1.1842
                    opacity: 0.1852
                    color: "#7d329d"
                    border {
                        width: 1872
                        color: "#8232c2"
                    }
                    shadow {
                        color: "#8732e7"
                        scale: 1.1912 // final assignment
                        verticalOffset: 1922
                        horizontalOffset: 1932
                        blur: 1942
                    }
                    image {
                        color: "#8c320c"
                    }
                    foreground {
                        width: 1962
                        height: 1972
                        minimumWidth: 1982
                        margins: 2002 // final assignment
                        leftMargin: 2012
                        rightMargin: 2022
                        topMargin: 2032
                        bottomMargin: 2042
                        radius: 2052
                        topLeftRadius: 2062
                        topRightRadius: 2072
                        bottomLeftRadius: 2082
                        bottomRightRadius: 2092
                        rotation: 2102
                        scale: 1.2112
                        opacity: 0.2122
                        color: "#913231"
                        border {
                            width: 2142
                            color: "#963256"
                        }
                        shadow {
                            color: "#9b327b"
                            scale: 1.2182 // final assignment
                            verticalOffset: 2192
                            horizontalOffset: 2202
                            blur: 2212
                        }
                        image {
                            color: "#a032a0"
                        }
                    }
                }
                second {
                    width: 2232
                    height: 2242
                    minimumWidth: 2252
                    margins: 2272 // final assignment
                    leftMargin: 2282
                    rightMargin: 2292
                    topMargin: 2302
                    bottomMargin: 2312
                    radius: 2322
                    topLeftRadius: 2332
                    topRightRadius: 2342
                    bottomLeftRadius: 2352
                    bottomRightRadius: 2362
                    rotation: 2372
                    scale: 1.2382
                    opacity: 0.2392
                    color: "#a532c5"
                    border {
                        width: 2412
                        color: "#aa32ea"
                    }
                    shadow {
                        color: "#af320f"
                        scale: 1.2452 // final assignment
                        verticalOffset: 2462
                        horizontalOffset: 2472
                        blur: 2482
                    }
                    image {
                        color: "#b43234"
                    }
                    foreground {
                        width: 2502
                        height: 2512
                        minimumWidth: 2522
                        margins: 2542 // final assignment
                        leftMargin: 2552
                        rightMargin: 2562
                        topMargin: 2572
                        bottomMargin: 2582
                        radius: 2592
                        topLeftRadius: 2602
                        topRightRadius: 2612
                        bottomLeftRadius: 2622
                        bottomRightRadius: 2632
                        rotation: 2642
                        scale: 1.2652
                        opacity: 0.2662
                        color: "#b93259"
                        border {
                            width: 2682
                            color: "#be327e"
                        }
                        shadow {
                            color: "#c332a3"
                            scale: 1.2722 // final assignment
                            verticalOffset: 2732
                            horizontalOffset: 2742
                            blur: 2752
                        }
                        image {
                            color: "#c832c8"
                        }
                    }
                }
            }
            text {
                color: "#cd32ed"
                pointSize: 2782
                padding: 2792
                leftPadding: 2802
                rightPadding: 2812
                topPadding: 2822
                bottomPadding: 2832
                first {
                    color: "#d23212"
                    pointSize: 2852
                    padding: 2862
                    leftPadding: 2872
                    rightPadding: 2882
                    topPadding: 2892
                    bottomPadding: 2902
                }
                second {
                    color: "#d73237"
                    pointSize: 2922
                    padding: 2932
                    leftPadding: 2942
                    rightPadding: 2952
                    topPadding: 2962
                    bottomPadding: 2972
                }
            }
        }

        button { // level 3
            leftPadding: 33 // final assignment
            rightPadding: 43
            topPadding: 53
            bottomPadding: 63
            background {
                width: 73
                height: 83
                minimumWidth: 93
                leftMargin: 123 // final assignment
                rightMargin: 133
                topMargin: 143
                bottomMargin: 153
                radius: 163
                topLeftRadius: 173
                topRightRadius: 183
                bottomLeftRadius: 193
                bottomRightRadius: 203
                rotation: 213 // final assignment
                scale: 1.0223
                opacity: 0.0233
                color: "#054b25"
                border {
                    width: 253
                    color: "#0a4b4a"
                }
                shadow {
                    color: "#0f4b6f"
                    verticalOffset: 303 // final assignment
                    horizontalOffset: 313
                    blur: 323
                }
                image {
                    color: "#144b94"
                }
            }
            handle {
                width: 343
                height: 353
                minimumWidth: 363
                leftMargin: 393 // final assignment
                rightMargin: 403
                topMargin: 413
                bottomMargin: 423
                radius: 433
                topLeftRadius: 443
                topRightRadius: 453
                bottomLeftRadius: 463
                bottomRightRadius: 473
                rotation: 483 // final assignment
                scale: 1.0493
                opacity: 0.0503
                color: "#194bb9"
                border {
                    width: 523
                    color: "#1e4bde"
                }
                shadow {
                    color: "#234b03"
                    verticalOffset: 573 // final assignment
                    horizontalOffset: 583
                    blur: 593
                }
                image {
                    color: "#284b28"
                }
                first {
                    width: 613
                    height: 623
                    minimumWidth: 633
                    leftMargin: 663 // final assignment
                    rightMargin: 673
                    topMargin: 683
                    bottomMargin: 693
                    radius: 703
                    topLeftRadius: 713
                    topRightRadius: 723
                    bottomLeftRadius: 733
                    bottomRightRadius: 743
                    rotation: 753 // final assignment
                    scale: 1.0763
                    opacity: 0.0773
                    color: "#2d4b4d"
                    border {
                        width: 793
                        color: "#324b72"
                    }
                    shadow {
                        color: "#374b97"
                        verticalOffset: 843 // final assignment
                        horizontalOffset: 853
                        blur: 863
                    }
                    image {
                        color: "#3c4bbc"
                    }
                }
                second {
                    width: 883
                    height: 893
                    minimumWidth: 903
                    leftMargin: 933 // final assignment
                    rightMargin: 943
                    topMargin: 953
                    bottomMargin: 963
                    radius: 973
                    topLeftRadius: 983
                    topRightRadius: 993
                    bottomLeftRadius: 1003
                    bottomRightRadius: 1013
                    rotation: 1023 // final assignment
                    scale: 1.1033
                    opacity: 0.1043
                    color: "#414be1"
                    border {
                        width: 1063
                        color: "#464b06"
                    }
                    shadow {
                        color: "#4b4b2b"
                        verticalOffset: 1113 // final assignment
                        horizontalOffset: 1123
                        blur: 1133
                    }
                    image {
                        color: "#504b50"
                    }
                }
            }
            indicator {
                width: 1153
                height: 1163
                minimumWidth: 1173
                leftMargin: 1203 // final assignment
                rightMargin: 1213
                topMargin: 1223
                bottomMargin: 1233
                radius: 1243
                topLeftRadius: 1253
                topRightRadius: 1263
                bottomLeftRadius: 1273
                bottomRightRadius: 1283
                rotation: 1293 // final assignment
                scale: 1.1303
                opacity: 0.1313
                color: "#554b75"
                border {
                    width: 1333
                    color: "#5a4b9a"
                }
                shadow {
                    color: "#5f4bbf"
                    verticalOffset: 1383 // final assignment
                    horizontalOffset: 1393
                    blur: 1403
                }
                image {
                    color: "#644be4"
                }
                foreground {
                    width: 1423
                    height: 1433
                    minimumWidth: 1443
                    leftMargin: 1473 // final assignment
                    rightMargin: 1483
                    topMargin: 1493
                    bottomMargin: 1503
                    radius: 1513
                    topLeftRadius: 1523
                    topRightRadius: 1533
                    bottomLeftRadius: 1543
                    bottomRightRadius: 1553
                    rotation: 1563 // final assignment
                    scale: 1.1573
                    opacity: 0.1583
                    color: "#694b09"
                    border {
                        width: 1603
                        color: "#6e4b2e"
                    }
                    shadow {
                        color: "#734b53"
                        verticalOffset: 1653 // final assignment
                        horizontalOffset: 1663
                        blur: 1673
                    }
                    image {
                        color: "#784b78"
                    }
                }
                first {
                    width: 1693
                    height: 1703
                    minimumWidth: 1713
                    leftMargin: 1743 // final assignment
                    rightMargin: 1753
                    topMargin: 1763
                    bottomMargin: 1773
                    radius: 1783
                    topLeftRadius: 1793
                    topRightRadius: 1803
                    bottomLeftRadius: 1813
                    bottomRightRadius: 1823
                    rotation: 1833 // final assignment
                    scale: 1.1843
                    opacity: 0.1853
                    color: "#7d4b9d"
                    border {
                        width: 1873
                        color: "#824bc2"
                    }
                    shadow {
                        color: "#874be7"
                        verticalOffset: 1923 // final assignment
                        horizontalOffset: 1933
                        blur: 1943
                    }
                    image {
                        color: "#8c4b0c"
                    }
                    foreground {
                        width: 1963
                        height: 1973
                        minimumWidth: 1983
                        leftMargin: 2013 // final assignment
                        rightMargin: 2023
                        topMargin: 2033
                        bottomMargin: 2043
                        radius: 2053
                        topLeftRadius: 2063
                        topRightRadius: 2073
                        bottomLeftRadius: 2083
                        bottomRightRadius: 2093
                        rotation: 2103 // final assignment
                        scale: 1.2113
                        opacity: 0.2123
                        color: "#914b31"
                        border {
                            width: 2143
                            color: "#964b56"
                        }
                        shadow {
                            color: "#9b4b7b"
                            verticalOffset: 2193 // final assignment
                            horizontalOffset: 2203
                            blur: 2213
                        }
                        image {
                            color: "#a04ba0"
                        }
                    }
                }
                second {
                    width: 2233
                    height: 2243
                    minimumWidth: 2253
                    leftMargin: 2283 // final assignment
                    rightMargin: 2293
                    topMargin: 2303
                    bottomMargin: 2313
                    radius: 2323
                    topLeftRadius: 2333
                    topRightRadius: 2343
                    bottomLeftRadius: 2353
                    bottomRightRadius: 2363
                    rotation: 2373 // final assignment
                    scale: 1.2383
                    opacity: 0.2393
                    color: "#a54bc5"
                    border {
                        width: 2413
                        color: "#aa4bea"
                    }
                    shadow {
                        color: "#af4b0f"
                        verticalOffset: 2463 // final assignment
                        horizontalOffset: 2473
                        blur: 2483
                    }
                    image {
                        color: "#b44b34"
                    }
                    foreground {
                        width: 2503
                        height: 2513
                        minimumWidth: 2523
                        leftMargin: 2553 // final assignment
                        rightMargin: 2563
                        topMargin: 2573
                        bottomMargin: 2583
                        radius: 2593
                        topLeftRadius: 2603
                        topRightRadius: 2613
                        bottomLeftRadius: 2623
                        bottomRightRadius: 2633
                        rotation: 2643 // final assignment
                        scale: 1.2653
                        opacity: 0.2663
                        color: "#b94b59"
                        border {
                            width: 2683
                            color: "#be4b7e"
                        }
                        shadow {
                            color: "#c34ba3"
                            verticalOffset: 2733 // final assignment
                            horizontalOffset: 2743
                            blur: 2753
                        }
                        image {
                            color: "#c84bc8"
                        }
                    }
                }
            }
            text {
                color: "#cd4bed"
                pointSize: 2783
                padding: 2793
                leftPadding: 2803
                rightPadding: 2813
                topPadding: 2823
                bottomPadding: 2833
                first {
                    color: "#d24b12"
                    pointSize: 2853
                    padding: 2863
                    leftPadding: 2873
                    rightPadding: 2883
                    topPadding: 2893
                    bottomPadding: 2903
                }
                second {
                    color: "#d74b37"
                    pointSize: 2923
                    padding: 2933
                    leftPadding: 2943
                    rightPadding: 2953
                    topPadding: 2963
                    bottomPadding: 2973
                }
            }
        }
    }

    control { // level 4
        rightPadding: 44 // final assignment
        topPadding: 54
        bottomPadding: 64
        background {
            width: 74
            height: 84
            minimumWidth: 94
            rightMargin: 134 // final assignment
            topMargin: 144
            bottomMargin: 154
            radius: 164
            topLeftRadius: 174
            topRightRadius: 184
            bottomLeftRadius: 194
            bottomRightRadius: 204
            scale: 1.0224 // final assignment
            opacity: 0.0234
            color: "#056425"
            border {
                width: 254
                color: "#0a644a"
            }
            shadow {
                color: "#0f646f"
                horizontalOffset: 314 // final assignment
                blur: 324
            }
            image {
                color: "#146494"
            }
        }
        handle {
            width: 344
            height: 354
            minimumWidth: 364
            rightMargin: 404 // final assignment
            topMargin: 414
            bottomMargin: 424
            radius: 434
            topLeftRadius: 444
            topRightRadius: 454
            bottomLeftRadius: 464
            bottomRightRadius: 474
            scale: 1.0494 // final assignment
            opacity: 0.0504
            color: "#1964b9"
            border {
                width: 524
                color: "#1e64de"
            }
            shadow {
                color: "#236403"
                horizontalOffset: 584 // final assignment
                blur: 594
            }
            image {
                color: "#286428"
            }
            first {
                width: 614
                height: 624
                minimumWidth: 634
                rightMargin: 674 // final assignment
                topMargin: 684
                bottomMargin: 694
                radius: 704
                topLeftRadius: 714
                topRightRadius: 724
                bottomLeftRadius: 734
                bottomRightRadius: 744
                scale: 1.0764 // final assignment
                opacity: 0.0774
                color: "#2d644d"
                border {
                    width: 794
                    color: "#326472"
                }
                shadow {
                    color: "#376497"
                    horizontalOffset: 854 // final assignment
                    blur: 864
                }
                image {
                    color: "#3c64bc"
                }
            }
            second {
                width: 884
                height: 894
                minimumWidth: 904
                rightMargin: 944 // final assignment
                topMargin: 954
                bottomMargin: 964
                radius: 974
                topLeftRadius: 984
                topRightRadius: 994
                bottomLeftRadius: 1004
                bottomRightRadius: 1014
                scale: 1.1034 // final assignment
                opacity: 0.1044
                color: "#4164e1"
                border {
                    width: 1064
                    color: "#466406"
                }
                shadow {
                    color: "#4b642b"
                    horizontalOffset: 1124 // final assignment
                    blur: 1134
                }
                image {
                    color: "#506450"
                }
            }
        }
        indicator {
            width: 1154
            height: 1164
            minimumWidth: 1174
            rightMargin: 1214 // final assignment
            topMargin: 1224
            bottomMargin: 1234
            radius: 1244
            topLeftRadius: 1254
            topRightRadius: 1264
            bottomLeftRadius: 1274
            bottomRightRadius: 1284
            scale: 1.1304 // final assignment
            opacity: 0.1314
            color: "#556475"
            border {
                width: 1334
                color: "#5a649a"
            }
            shadow {
                color: "#5f64bf"
                horizontalOffset: 1394 // final assignment
                blur: 1404
            }
            image {
                color: "#6464e4"
            }
            foreground {
                width: 1424
                height: 1434
                minimumWidth: 1444
                rightMargin: 1484 // final assignment
                topMargin: 1494
                bottomMargin: 1504
                radius: 1514
                topLeftRadius: 1524
                topRightRadius: 1534
                bottomLeftRadius: 1544
                bottomRightRadius: 1554
                scale: 1.1574 // final assignment
                opacity: 0.1584
                color: "#696409"
                border {
                    width: 1604
                    color: "#6e642e"
                }
                shadow {
                    color: "#736453"
                    horizontalOffset: 1664 // final assignment
                    blur: 1674
                }
                image {
                    color: "#786478"
                }
            }
            first {
                width: 1694
                height: 1704
                minimumWidth: 1714
                rightMargin: 1754 // final assignment
                topMargin: 1764
                bottomMargin: 1774
                radius: 1784
                topLeftRadius: 1794
                topRightRadius: 1804
                bottomLeftRadius: 1814
                bottomRightRadius: 1824
                scale: 1.1844 // final assignment
                opacity: 0.1854
                color: "#7d649d"
                border {
                    width: 1874
                    color: "#8264c2"
                }
                shadow {
                    color: "#8764e7"
                    horizontalOffset: 1934 // final assignment
                    blur: 1944
                }
                image {
                    color: "#8c640c"
                }
                foreground {
                    width: 1964
                    height: 1974
                    minimumWidth: 1984
                    rightMargin: 2024 // final assignment
                    topMargin: 2034
                    bottomMargin: 2044
                    radius: 2054
                    topLeftRadius: 2064
                    topRightRadius: 2074
                    bottomLeftRadius: 2084
                    bottomRightRadius: 2094
                    scale: 1.2114 // final assignment
                    opacity: 0.2124
                    color: "#916431"
                    border {
                        width: 2144
                        color: "#966456"
                    }
                    shadow {
                        color: "#9b647b"
                        horizontalOffset: 2204 // final assignment
                        blur: 2214
                    }
                    image {
                        color: "#a064a0"
                    }
                }
            }
            second {
                width: 2234
                height: 2244
                minimumWidth: 2254
                rightMargin: 2294 // final assignment
                topMargin: 2304
                bottomMargin: 2314
                radius: 2324
                topLeftRadius: 2334
                topRightRadius: 2344
                bottomLeftRadius: 2354
                bottomRightRadius: 2364
                scale: 1.2384 // final assignment
                opacity: 0.2394
                color: "#a564c5"
                border {
                    width: 2414
                    color: "#aa64ea"
                }
                shadow {
                    color: "#af640f"
                    horizontalOffset: 2474 // final assignment
                    blur: 2484
                }
                image {
                    color: "#b46434"
                }
                foreground {
                    width: 2504
                    height: 2514
                    minimumWidth: 2524
                    rightMargin: 2564 // final assignment
                    topMargin: 2574
                    bottomMargin: 2584
                    radius: 2594
                    topLeftRadius: 2604
                    topRightRadius: 2614
                    bottomLeftRadius: 2624
                    bottomRightRadius: 2634
                    scale: 1.2654 // final assignment
                    opacity: 0.2664
                    color: "#b96459"
                    border {
                        width: 2684
                        color: "#be647e"
                    }
                    shadow {
                        color: "#c364a3"
                        horizontalOffset: 2744 // final assignment
                        blur: 2754
                    }
                    image {
                        color: "#c864c8"
                    }
                }
            }
        }
        text {
            color: "#cd64ed"
            pointSize: 2784
            padding: 2794
            leftPadding: 2804
            rightPadding: 2814
            topPadding: 2824
            bottomPadding: 2834
            first {
                color: "#d26412"
                pointSize: 2854
                padding: 2864
                leftPadding: 2874
                rightPadding: 2884
                topPadding: 2894
                bottomPadding: 2904
            }
            second {
                color: "#d76437"
                pointSize: 2924
                padding: 2934
                leftPadding: 2944
                rightPadding: 2954
                topPadding: 2964
                bottomPadding: 2974
            }
        }
    }

    abstractButton { // level 5
        topPadding: 55 // final assignment
        bottomPadding: 65
        background {
            width: 75
            height: 85
            minimumWidth: 95
            topMargin: 145 // final assignment
            bottomMargin: 155
            radius: 165
            topLeftRadius: 175
            topRightRadius: 185
            bottomLeftRadius: 195
            bottomRightRadius: 205
            opacity: 0.0235 // final assignment
            color: "#057d25"
            border {
                width: 255
                color: "#0a7d4a"
            }
            shadow {
                color: "#0f7d6f"
                blur: 325 // final assignment
            }
            image {
                color: "#147d94"
            }
        }
        handle {
            width: 345
            height: 355
            minimumWidth: 365
            topMargin: 415 // final assignment
            bottomMargin: 425
            radius: 435
            topLeftRadius: 445
            topRightRadius: 455
            bottomLeftRadius: 465
            bottomRightRadius: 475
            opacity: 0.0505 // final assignment
            color: "#197db9"
            border {
                width: 525
                color: "#1e7dde"
            }
            shadow {
                color: "#237d03"
                blur: 595 // final assignment
            }
            image {
                color: "#287d28"
            }
            first {
                width: 615
                height: 625
                minimumWidth: 635
                topMargin: 685 // final assignment
                bottomMargin: 695
                radius: 705
                topLeftRadius: 715
                topRightRadius: 725
                bottomLeftRadius: 735
                bottomRightRadius: 745
                opacity: 0.0775 // final assignment
                color: "#2d7d4d"
                border {
                    width: 795
                    color: "#327d72"
                }
                shadow {
                    color: "#377d97"
                    blur: 865 // final assignment
                }
                image {
                    color: "#3c7dbc"
                }
            }
            second {
                width: 885
                height: 895
                minimumWidth: 905
                topMargin: 955 // final assignment
                bottomMargin: 965
                radius: 975
                topLeftRadius: 985
                topRightRadius: 995
                bottomLeftRadius: 1005
                bottomRightRadius: 1015
                opacity: 0.1045 // final assignment
                color: "#417de1"
                border {
                    width: 1065
                    color: "#467d06"
                }
                shadow {
                    color: "#4b7d2b"
                    blur: 1135 // final assignment
                }
                image {
                    color: "#507d50"
                }
            }
        }
        indicator {
            width: 1155
            height: 1165
            minimumWidth: 1175
            topMargin: 1225 // final assignment
            bottomMargin: 1235
            radius: 1245
            topLeftRadius: 1255
            topRightRadius: 1265
            bottomLeftRadius: 1275
            bottomRightRadius: 1285
            opacity: 0.1315 // final assignment
            color: "#557d75"
            border {
                width: 1335
                color: "#5a7d9a"
            }
            shadow {
                color: "#5f7dbf"
                blur: 1405 // final assignment
            }
            image {
                color: "#647de4"
            }
            foreground {
                width: 1425
                height: 1435
                minimumWidth: 1445
                topMargin: 1495 // final assignment
                bottomMargin: 1505
                radius: 1515
                topLeftRadius: 1525
                topRightRadius: 1535
                bottomLeftRadius: 1545
                bottomRightRadius: 1555
                opacity: 0.1585 // final assignment
                color: "#697d09"
                border {
                    width: 1605
                    color: "#6e7d2e"
                }
                shadow {
                    color: "#737d53"
                    blur: 1675 // final assignment
                }
                image {
                    color: "#787d78"
                }
            }
            first {
                width: 1695
                height: 1705
                minimumWidth: 1715
                topMargin: 1765 // final assignment
                bottomMargin: 1775
                radius: 1785
                topLeftRadius: 1795
                topRightRadius: 1805
                bottomLeftRadius: 1815
                bottomRightRadius: 1825
                opacity: 0.1855 // final assignment
                color: "#7d7d9d"
                border {
                    width: 1875
                    color: "#827dc2"
                }
                shadow {
                    color: "#877de7"
                    blur: 1945 // final assignment
                }
                image {
                    color: "#8c7d0c"
                }
                foreground {
                    width: 1965
                    height: 1975
                    minimumWidth: 1985
                    topMargin: 2035 // final assignment
                    bottomMargin: 2045
                    radius: 2055
                    topLeftRadius: 2065
                    topRightRadius: 2075
                    bottomLeftRadius: 2085
                    bottomRightRadius: 2095
                    opacity: 0.2125 // final assignment
                    color: "#917d31"
                    border {
                        width: 2145
                        color: "#967d56"
                    }
                    shadow {
                        color: "#9b7d7b"
                        blur: 2215 // final assignment
                    }
                    image {
                        color: "#a07da0"
                    }
                }
            }
            second {
                width: 2235
                height: 2245
                minimumWidth: 2255
                topMargin: 2305 // final assignment
                bottomMargin: 2315
                radius: 2325
                topLeftRadius: 2335
                topRightRadius: 2345
                bottomLeftRadius: 2355
                bottomRightRadius: 2365
                opacity: 0.2395 // final assignment
                color: "#a57dc5"
                border {
                    width: 2415
                    color: "#aa7dea"
                }
                shadow {
                    color: "#af7d0f"
                    blur: 2485 // final assignment
                }
                image {
                    color: "#b47d34"
                }
                foreground {
                    width: 2505
                    height: 2515
                    minimumWidth: 2525
                    topMargin: 2575 // final assignment
                    bottomMargin: 2585
                    radius: 2595
                    topLeftRadius: 2605
                    topRightRadius: 2615
                    bottomLeftRadius: 2625
                    bottomRightRadius: 2635
                    opacity: 0.2665 // final assignment
                    color: "#b97d59"
                    border {
                        width: 2685
                        color: "#be7d7e"
                    }
                    shadow {
                        color: "#c37da3"
                        blur: 2755 // final assignment
                    }
                    image {
                        color: "#c87dc8"
                    }
                }
            }
        }
        text {
            color: "#cd7ded"
            pointSize: 2785
            padding: 2795
            leftPadding: 2805
            rightPadding: 2815
            topPadding: 2825
            bottomPadding: 2835
            first {
                color: "#d27d12"
                pointSize: 2855
                padding: 2865
                leftPadding: 2875
                rightPadding: 2885
                topPadding: 2895
                bottomPadding: 2905
            }
            second {
                color: "#d77d37"
                pointSize: 2925
                padding: 2935
                leftPadding: 2945
                rightPadding: 2955
                topPadding: 2965
                bottomPadding: 2975
            }
        }
    }

    button { // level 6
        bottomPadding: 66 // final assignment
        background {
            width: 76
            height: 86
            minimumWidth: 96
            bottomMargin: 156 // final assignment
            radius: 166
            topLeftRadius: 176
            topRightRadius: 186
            bottomLeftRadius: 196
            bottomRightRadius: 206
            color: "#059625" // final assignment
            border {
                width: 256
                color: "#0a964a"
            }
            shadow {
                color: "#0f966f"
            }
            image {
                color: "#149694" // final assignment
            }
        }
        handle {
            width: 346
            height: 356
            minimumWidth: 366
            bottomMargin: 426 // final assignment
            radius: 436
            topLeftRadius: 446
            topRightRadius: 456
            bottomLeftRadius: 466
            bottomRightRadius: 476
            color: "#1996b9" // final assignment
            border {
                width: 526
                color: "#1e96de"
            }
            shadow {
                color: "#239603"
            }
            image {
                color: "#289628" // final assignment
            }
            first {
                width: 616
                height: 626
                minimumWidth: 636
                bottomMargin: 696 // final assignment
                radius: 706
                topLeftRadius: 716
                topRightRadius: 726
                bottomLeftRadius: 736
                bottomRightRadius: 746
                color: "#2d964d" // final assignment
                border {
                    width: 796
                    color: "#329672"
                }
                shadow {
                    color: "#379697"
                }
                image {
                    color: "#3c96bc" // final assignment
                }
            }
            second {
                width: 886
                height: 896
                minimumWidth: 906
                bottomMargin: 966 // final assignment
                radius: 976
                topLeftRadius: 986
                topRightRadius: 996
                bottomLeftRadius: 1006
                bottomRightRadius: 1016
                color: "#4196e1" // final assignment
                border {
                    width: 1066
                    color: "#469606"
                }
                shadow {
                    color: "#4b962b"
                }
                image {
                    color: "#509650" // final assignment
                }
            }
        }
        indicator {
            width: 1156
            height: 1166
            minimumWidth: 1176
            bottomMargin: 1236 // final assignment
            radius: 1246
            topLeftRadius: 1256
            topRightRadius: 1266
            bottomLeftRadius: 1276
            bottomRightRadius: 1286
            color: "#559675" // final assignment
            border {
                width: 1336
                color: "#5a969a"
            }
            shadow {
                color: "#5f96bf"
            }
            image {
                color: "#6496e4" // final assignment
            }
            foreground {
                width: 1426
                height: 1436
                minimumWidth: 1446
                bottomMargin: 1506 // final assignment
                radius: 1516
                topLeftRadius: 1526
                topRightRadius: 1536
                bottomLeftRadius: 1546
                bottomRightRadius: 1556
                color: "#699609" // final assignment
                border {
                    width: 1606
                    color: "#6e962e"
                }
                shadow {
                    color: "#739653"
                }
                image {
                    color: "#789678" // final assignment
                }
            }
            first {
                width: 1696
                height: 1706
                minimumWidth: 1716
                bottomMargin: 1776 // final assignment
                radius: 1786
                topLeftRadius: 1796
                topRightRadius: 1806
                bottomLeftRadius: 1816
                bottomRightRadius: 1826
                color: "#7d969d" // final assignment
                border {
                    width: 1876
                    color: "#8296c2"
                }
                shadow {
                    color: "#8796e7"
                }
                image {
                    color: "#8c960c" // final assignment
                }
                foreground {
                    width: 1966
                    height: 1976
                    minimumWidth: 1986
                    bottomMargin: 2046 // final assignment
                    radius: 2056
                    topLeftRadius: 2066
                    topRightRadius: 2076
                    bottomLeftRadius: 2086
                    bottomRightRadius: 2096
                    color: "#919631" // final assignment
                    border {
                        width: 2146
                        color: "#969656"
                    }
                    shadow {
                        color: "#9b967b"
                    }
                    image {
                        color: "#a096a0" // final assignment
                    }
                }
            }
            second {
                width: 2236
                height: 2246
                minimumWidth: 2256
                bottomMargin: 2316 // final assignment
                radius: 2326
                topLeftRadius: 2336
                topRightRadius: 2346
                bottomLeftRadius: 2356
                bottomRightRadius: 2366
                color: "#a596c5" // final assignment
                border {
                    width: 2416
                    color: "#aa96ea"
                }
                shadow {
                    color: "#af960f"
                }
                image {
                    color: "#b49634" // final assignment
                }
                foreground {
                    width: 2506
                    height: 2516
                    minimumWidth: 2526
                    bottomMargin: 2586 // final assignment
                    radius: 2596
                    topLeftRadius: 2606
                    topRightRadius: 2616
                    bottomLeftRadius: 2626
                    bottomRightRadius: 2636
                    color: "#b99659" // final assignment
                    border {
                        width: 2686
                        color: "#be967e"
                    }
                    shadow {
                        color: "#c396a3"
                    }
                    image {
                        color: "#c896c8" // final assignment
                    }
                }
            }
        }
        text {
            color: "#cd96ed"
            pointSize: 2786
            padding: 2796
            leftPadding: 2806
            rightPadding: 2816
            topPadding: 2826
            bottomPadding: 2836
            first {
                color: "#d29612"
                pointSize: 2856
                padding: 2866
                leftPadding: 2876
                rightPadding: 2886
                topPadding: 2896
                bottomPadding: 2906
            }
            second {
                color: "#d79637"
                pointSize: 2926
                padding: 2936
                leftPadding: 2946
                rightPadding: 2956
                topPadding: 2966
                bottomPadding: 2976
            }
        }
    }

    light: Theme {
        control { // level 7
            background {
                width: 77 // final assignment
                height: 87
                minimumWidth: 97
                radius: 167 // final assignment
                topLeftRadius: 177
                topRightRadius: 187
                bottomLeftRadius: 197 // final assignment
                bottomRightRadius: 207 // final assignment
                border {
                    width: 257 // final assignment
                    color: "#0aaf4a"
                }
                shadow {
                    color: "#0faf6f"
                }
            }
            handle {
                width: 347 // final assignment
                height: 357
                minimumWidth: 367
                radius: 437 // final assignment
                topLeftRadius: 447
                topRightRadius: 457
                bottomLeftRadius: 467 // final assignment
                bottomRightRadius: 477 // final assignment
                border {
                    width: 527 // final assignment
                    color: "#1eafde"
                }
                shadow {
                    color: "#23af03"
                }
                first {
                    width: 617 // final assignment
                    height: 627
                    minimumWidth: 637
                    radius: 707 // final assignment
                    topLeftRadius: 717
                    topRightRadius: 727
                    bottomLeftRadius: 737 // final assignment
                    bottomRightRadius: 747 // final assignment
                    border {
                        width: 797 // final assignment
                        color: "#32af72"
                    }
                    shadow {
                        color: "#37af97"
                    }
                }
                second {
                    width: 887 // final assignment
                    height: 897
                    minimumWidth: 907
                    radius: 977 // final assignment
                    topLeftRadius: 987
                    topRightRadius: 997
                    bottomLeftRadius: 1007 // final assignment
                    bottomRightRadius: 1017 // final assignment
                    border {
                        width: 1067 // final assignment
                        color: "#46af06"
                    }
                    shadow {
                        color: "#4baf2b"
                    }
                }
            }
            indicator {
                width: 1157 // final assignment
                height: 1167
                minimumWidth: 1177
                radius: 1247 // final assignment
                topLeftRadius: 1257
                topRightRadius: 1267
                bottomLeftRadius: 1277 // final assignment
                bottomRightRadius: 1287 // final assignment
                border {
                    width: 1337 // final assignment
                    color: "#5aaf9a"
                }
                shadow {
                    color: "#5fafbf"
                }
                foreground {
                    width: 1427 // final assignment
                    height: 1437
                    minimumWidth: 1447
                    radius: 1517 // final assignment
                    topLeftRadius: 1527
                    topRightRadius: 1537
                    bottomLeftRadius: 1547 // final assignment
                    bottomRightRadius: 1557 // final assignment
                    border {
                        width: 1607 // final assignment
                        color: "#6eaf2e"
                    }
                    shadow {
                        color: "#73af53"
                    }
                }
                first {
                    width: 1697 // final assignment
                    height: 1707
                    minimumWidth: 1717
                    radius: 1787 // final assignment
                    topLeftRadius: 1797
                    topRightRadius: 1807
                    bottomLeftRadius: 1817 // final assignment
                    bottomRightRadius: 1827 // final assignment
                    border {
                        width: 1877 // final assignment
                        color: "#82afc2"
                    }
                    shadow {
                        color: "#87afe7"
                    }
                    foreground {
                        width: 1967 // final assignment
                        height: 1977
                        minimumWidth: 1987
                        radius: 2057 // final assignment
                        topLeftRadius: 2067
                        topRightRadius: 2077
                        bottomLeftRadius: 2087 // final assignment
                        bottomRightRadius: 2097 // final assignment
                        border {
                            width: 2147 // final assignment
                            color: "#96af56"
                        }
                        shadow {
                            color: "#9baf7b"
                        }
                    }
                }
                second {
                    width: 2237 // final assignment
                    height: 2247
                    minimumWidth: 2257
                    radius: 2327 // final assignment
                    topLeftRadius: 2337
                    topRightRadius: 2347
                    bottomLeftRadius: 2357 // final assignment
                    bottomRightRadius: 2367 // final assignment
                    border {
                        width: 2417 // final assignment
                        color: "#aaafea"
                    }
                    shadow {
                        color: "#afaf0f"
                    }
                    foreground {
                        width: 2507 // final assignment
                        height: 2517
                        minimumWidth: 2527
                        radius: 2597 // final assignment
                        topLeftRadius: 2607
                        topRightRadius: 2617
                        bottomLeftRadius: 2627 // final assignment
                        bottomRightRadius: 2637 // final assignment
                        border {
                            width: 2687 // final assignment
                            color: "#beaf7e"
                        }
                        shadow {
                            color: "#c3afa3"
                        }
                    }
                }
            }
            text {
                color: "#cdafed" // final assignment
                pointSize: 2787
                padding: 2797
                leftPadding: 2807
                rightPadding: 2817
                topPadding: 2827
                bottomPadding: 2837
                first {
                    color: "#d2af12" // final assignment
                    pointSize: 2857
                    padding: 2867
                    leftPadding: 2877
                    rightPadding: 2887
                    topPadding: 2897
                    bottomPadding: 2907
                }
                second {
                    color: "#d7af37" // final assignment
                    pointSize: 2927
                    padding: 2937
                    leftPadding: 2947
                    rightPadding: 2957
                    topPadding: 2967
                    bottomPadding: 2977
                }
            }
        }

        abstractButton { // level 8
            background {
                height: 88 // final assignment
                minimumWidth: 98
                topLeftRadius: 178 // final assignment
                topRightRadius: 188
                border {
                    color: "#0ac84a" // final assignment
                }
                shadow {
                    color: "#0fc86f"
                }
            }
            handle {
                height: 358 // final assignment
                minimumWidth: 368
                topLeftRadius: 448 // final assignment
                topRightRadius: 458
                border {
                    color: "#1ec8de" // final assignment
                }
                shadow {
                    color: "#23c803"
                }
                first {
                    height: 628 // final assignment
                    minimumWidth: 638
                    topLeftRadius: 718 // final assignment
                    topRightRadius: 728
                    border {
                        color: "#32c872" // final assignment
                    }
                    shadow {
                        color: "#37c897"
                    }
                }
                second {
                    height: 898 // final assignment
                    minimumWidth: 908
                    topLeftRadius: 988 // final assignment
                    topRightRadius: 998
                    border {
                        color: "#46c806" // final assignment
                    }
                    shadow {
                        color: "#4bc82b"
                    }
                }
            }
            indicator {
                height: 1168 // final assignment
                minimumWidth: 1178
                topLeftRadius: 1258 // final assignment
                topRightRadius: 1268
                border {
                    color: "#5ac89a" // final assignment
                }
                shadow {
                    color: "#5fc8bf"
                }
                foreground {
                    height: 1438 // final assignment
                    minimumWidth: 1448
                    topLeftRadius: 1528 // final assignment
                    topRightRadius: 1538
                    border {
                        color: "#6ec82e" // final assignment
                    }
                    shadow {
                        color: "#73c853"
                    }
                }
                first {
                    height: 1708 // final assignment
                    minimumWidth: 1718
                    topLeftRadius: 1798 // final assignment
                    topRightRadius: 1808
                    border {
                        color: "#82c8c2" // final assignment
                    }
                    shadow {
                        color: "#87c8e7"
                    }
                    foreground {
                        height: 1978 // final assignment
                        minimumWidth: 1988
                        topLeftRadius: 2068 // final assignment
                        topRightRadius: 2078
                        border {
                            color: "#96c856" // final assignment
                        }
                        shadow {
                            color: "#9bc87b"
                        }
                    }
                }
                second {
                    height: 2248 // final assignment
                    minimumWidth: 2258
                    topLeftRadius: 2338 // final assignment
                    topRightRadius: 2348
                    border {
                        color: "#aac8ea" // final assignment
                    }
                    shadow {
                        color: "#afc80f"
                    }
                    foreground {
                        height: 2518 // final assignment
                        minimumWidth: 2528
                        topLeftRadius: 2608 // final assignment
                        topRightRadius: 2618
                        border {
                            color: "#bec87e" // final assignment
                        }
                        shadow {
                            color: "#c3c8a3"
                        }
                    }
                }
            }
            text {
                pointSize: 2788 // final assignment
                padding: 2798
                leftPadding: 2808
                rightPadding: 2818
                topPadding: 2828
                bottomPadding: 2838
                first {
                    pointSize: 2858 // final assignment
                    padding: 2868
                    leftPadding: 2878
                    rightPadding: 2888
                    topPadding: 2898
                    bottomPadding: 2908
                }
                second {
                    pointSize: 2928 // final assignment
                    padding: 2938
                    leftPadding: 2948
                    rightPadding: 2958
                    topPadding: 2968
                    bottomPadding: 2978
                }
            }
        }

        button { // level 9
            background {
                minimumWidth: 99 // final assignment
                topRightRadius: 189 // final assignment
                shadow {
                    color: "#0fe16f" // final assignment
                }
            }
            handle {
                minimumWidth: 369 // final assignment
                topRightRadius: 459 // final assignment
                shadow {
                    color: "#23e103" // final assignment
                }
                first {
                    minimumWidth: 639 // final assignment
                    topRightRadius: 729 // final assignment
                    shadow {
                        color: "#37e197" // final assignment
                    }
                }
                second {
                    minimumWidth: 909 // final assignment
                    topRightRadius: 999 // final assignment
                    shadow {
                        color: "#4be12b" // final assignment
                    }
                }
            }
            indicator {
                minimumWidth: 1179 // final assignment
                topRightRadius: 1269 // final assignment
                shadow {
                    color: "#5fe1bf" // final assignment
                }
                foreground {
                    minimumWidth: 1449 // final assignment
                    topRightRadius: 1539 // final assignment
                    shadow {
                        color: "#73e153" // final assignment
                    }
                }
                first {
                    minimumWidth: 1719 // final assignment
                    topRightRadius: 1809 // final assignment
                    shadow {
                        color: "#87e1e7" // final assignment
                    }
                    foreground {
                        minimumWidth: 1989 // final assignment
                        topRightRadius: 2079 // final assignment
                        shadow {
                            color: "#9be17b" // final assignment
                        }
                    }
                }
                second {
                    minimumWidth: 2259 // final assignment
                    topRightRadius: 2349 // final assignment
                    shadow {
                        color: "#afe10f" // final assignment
                    }
                    foreground {
                        minimumWidth: 2529 // final assignment
                        topRightRadius: 2619 // final assignment
                        shadow {
                            color: "#c3e1a3" // final assignment
                        }
                    }
                }
            }
            text {
                padding: 2799 // final assignment
                leftPadding: 2809 // final assignment
                rightPadding: 2819 // final assignment
                topPadding: 2829 // final assignment
                bottomPadding: 2839 // final assignment
                first {
                    padding: 2869 // final assignment
                    leftPadding: 2879 // final assignment
                    rightPadding: 2889 // final assignment
                    topPadding: 2899 // final assignment
                    bottomPadding: 2909 // final assignment
                }
                second {
                    padding: 2939 // final assignment
                    leftPadding: 2949 // final assignment
                    rightPadding: 2959 // final assignment
                    topPadding: 2969 // final assignment
                    bottomPadding: 2979 // final assignment
                }
            }
        }
    }
}
