TEMPLATE = aux

INSTALLER = setup
# macOS X settings
macx{
 INPUT = $$PWD/config/config.xml $$PWD/packages
}
# win settings
win32{
    message("WIN specific settings")
    INPUT = $$PWD\config\config.xml $$PWD\packages
}

CM_Setup.input = INPUT
CM_Setup.output = $$INSTALLER

macx{
    message("MacOSX specific settings")
    CM_Setup.commands = ../../../Qt/QtIFW-3.0.2/bin/binarycreator -c $$PWD/config/config.xml -p $$PWD/packages ${QMAKE_FILE_OUT}
}

# win settings
win32{
  CM_Setup.commands = c:/Qt/QtIFW-3.0.2/bin/binarycreator -c $$PWD/config/config.xml -p $$PWD/packages ${QMAKE_FILE_OUT}
 #   CM_Setup.commands =C:\Qt\QtIFW-3.0.2\bin\binarycreator -c $$PWD\config\config.xml -p $$PWD\packages ${QMAKE_FILE_OUT}
}
CM_Setup.CONFIG += target_predeps no_link combine

QMAKE_EXTRA_COMPILERS += CM_Setup

OTHER_FILES = README

DISTFILES += \
    packages/chronomodel_QtIFW.composant1/meta/package.xml \
    packages/chronomodel_QtIFW.composant1/meta/installscript.qs
