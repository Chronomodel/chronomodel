TEMPLATE = aux

INSTALLER = setup

INPUT = $$PWD/config/config.xml $$PWD/packages
CM_Setup.input = INPUT
CM_Setup.output = $$INSTALLER
CM_Setup.commands = ../../../Qt/QtIFW-3.0.2/bin/binarycreator -c $$PWD/config/config.xml -p $$PWD/packages ${QMAKE_FILE_OUT}
CM_Setup.CONFIG += target_predeps no_link combine

QMAKE_EXTRA_COMPILERS += CM_Setup

OTHER_FILES = README

DISTFILES += \
    packages/chronomodel_QtIFW.composant1/meta/package.xml \
    packages/chronomodel_QtIFW.composant1/meta/installscript.qs
