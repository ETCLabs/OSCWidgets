TARGET = OSCWidgets
TEMPLATE = app
QT += core widgets gui network
CONFIG += c++11

CONFIG(debug, debug|release) {
    DESTDIR = $$clean_path($${PWD}/build/Debug)
}

CONFIG(release, debug|release) {
    DESTDIR = $$clean_path($${PWD}/build/Release)
    CONFIG += force_debug_info separate_debug_info
    CONFIG += hide_symbols
}

HEADERS = \
   OSCWidgets/EditPanel.h \
   OSCWidgets/EosPlatform.h \
   OSCWidgets/FadeButton.h \
   OSCWidgets/LogFile.h \
   OSCWidgets/LogWidget.h \
   OSCWidgets/MainWindow.h \
   OSCWidgets/NetworkThreads.h \
   OSCWidgets/QtInclude.h \
   OSCWidgets/resource.h \
   OSCWidgets/SettingsPanel.h \
   OSCWidgets/Toy.h \
   OSCWidgets/ToyActivity.h \
   OSCWidgets/ToyButton.h \
   OSCWidgets/ToyCmd.h \
   OSCWidgets/ToyEncoder.h \
   OSCWidgets/ToyFlicker.h \
   OSCWidgets/ToyGrid.h \
   OSCWidgets/ToyLabel.h \
   OSCWidgets/ToyMath.h \
   OSCWidgets/ToyMetro.h \
   OSCWidgets/ToyPedal.h \
   OSCWidgets/Toys.h \
   OSCWidgets/ToySine.h \
   OSCWidgets/ToySlider.h \
   OSCWidgets/ToyWidget.h \
   OSCWidgets/ToyWindow.h \
   OSCWidgets/ToyXY.h \
   OSCWidgets/Utils.h \
   ../EosSyncLib/EosSyncLib/EosLog.h \
   ../EosSyncLib/EosSyncLib/EosOsc.h \
   ../EosSyncLib/EosSyncLib/EosSyncLib.h \
   ../EosSyncLib/EosSyncLib/EosTcp.h \
   ../EosSyncLib/EosSyncLib/EosTimer.h \
   ../EosSyncLib/EosSyncLib/EosUdp.h \
   ../EosSyncLib/EosSyncLib/OSCParser.h

SOURCES = \
   OSCWidgets/EditPanel.cpp \
   OSCWidgets/EosPlatform.cpp \
   OSCWidgets/FadeButton.cpp \
   OSCWidgets/LogFile.cpp \
   OSCWidgets/LogWidget.cpp \
   OSCWidgets/main.cpp \
   OSCWidgets/MainWindow.cpp \
   OSCWidgets/NetworkThreads.cpp \
   OSCWidgets/SettingsPanel.cpp \
   OSCWidgets/Toy.cpp \
   OSCWidgets/ToyActivity.cpp \
   OSCWidgets/ToyButton.cpp \
   OSCWidgets/ToyCmd.cpp \
   OSCWidgets/ToyEncoder.cpp \
   OSCWidgets/ToyFlicker.cpp \
   OSCWidgets/ToyGrid.cpp \
   OSCWidgets/ToyLabel.cpp \
   OSCWidgets/ToyMetro.cpp \
   OSCWidgets/ToyPedal.cpp \
   OSCWidgets/Toys.cpp \
   OSCWidgets/ToySine.cpp \
   OSCWidgets/ToySlider.cpp \
   OSCWidgets/ToyWidget.cpp \
   OSCWidgets/ToyWindow.cpp \
   OSCWidgets/ToyXY.cpp \
   OSCWidgets/Utils.cpp \
   ../EosSyncLib/EosSyncLib/EosLog.cpp \
   ../EosSyncLib/EosSyncLib/EosOsc.cpp \
   ../EosSyncLib/EosSyncLib/EosSyncLib.cpp \
   ../EosSyncLib/EosSyncLib/EosTcp.cpp \
   ../EosSyncLib/EosSyncLib/EosTimer.cpp \
   ../EosSyncLib/EosSyncLib/EosUdp.cpp \
   ../EosSyncLib/EosSyncLib/OSCParser.cpp

RESOURCES     = OSCWidgets/OSCWidgets.qrc

OBJECTIVE_SOURCES += OSCWidgets/EosPlatform_Mac_Native.mm
unix {
    HEADERS += \
        ../EosSyncLib/EosSyncLib/EosTcp_Mac.h \
        ../EosSyncLib/EosSyncLib/EosUdp_Mac.h


    SOURCES += \
        ../EosSyncLib/EosSyncLib/EosTcp_Mac.cpp \
        ../EosSyncLib/EosSyncLib/EosUdp_Mac.cpp
}
mac {
    HEADERS += \
        OSCWidgets/EosPlatform_Mac.h \
        OSCWidgets/EosPlatform_Mac_Bridge.h \
        OSCWidgets/EosPlatform_Mac_Native.h
    SOURCES += \
        OSCWidgets/EosPlatform_Mac.cpp

    LIBS += -framework Foundation

    ICON = eos.icns
    QMAKE_INFO_PLIST = Info.plist

    CONFIG(release, debug|release) {
        QMAKE_POST_LINK += $$clean_path("$$[QT_INSTALL_BINS]/macdeployqt") $$clean_path("$${DESTDIR}/$${TARGET}.app")
    }

    QMAKE_CLEAN += =r $$shell_quote($$clean_path($${DESTDIR}/$${TARGET}.app))
    QMAKE_CLEAN += =r $$shell_quote($$clean_path($${DESTDIR}/$${TARGET}.app.dSYM))
}

win32 {
    HEADERS += \
        ../EosSyncLib/EosSyncLib/EosTcp_Win.h \
        ../EosSyncLib/EosSyncLib/EosUdp_Win.h

    SOURCES += \
        ../EosSyncLib/EosSyncLib/EosTcp_Win.cpp \
        ../EosSyncLib/EosSyncLib/EosUdp_Win.cpp
    LIBS += -luser32 -lWs2_32
    QT += winextras
}

INCLUDEPATH = \
    OSCWidgets/. \
    ../EosSyncLib/EosSyncLib/
