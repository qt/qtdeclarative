TEMPLATE = subdirs

PRIVATETESTS += \
    qsgage \
    qsgangleddirection \
    qsgcumulativedirection \
    qsgcustomaffector \
    qsgcustomparticle \
    qsgellipseextruder \
    qsgfriction \
    qsggravity \
    qsgimageparticle \
    qsgitemparticle \
    qsglineextruder \
    qsgmaskextruder \
    qsgparticlegroup \
    qsgparticlesystem \
    qsgpointattractor \
    qsgpointdirection \
    qsgrectangleextruder \
    qsgtargetdirection \
    qsgtrailemitter \
    qsgturbulence \
    qsgwander

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
