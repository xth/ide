TARGET = AutoTest
TEMPLATE = lib

PROVIDER = Digia

include(../../qtcreatorplugin.pri)
include(autotest_dependencies.pri)

DEFINES += AUTOTEST_LIBRARY

SOURCES += \
    squishsettings.cpp \
    squishsettingspage.cpp \
    testtreeview.cpp \
    testtreemodel.cpp \
    testtreeitem.cpp \
    testvisitor.cpp \
    testinfo.cpp \
    testcodeparser.cpp \
    autotestplugin.cpp \
    testrunner.cpp \
    testconfiguration.cpp \
    testresult.cpp \
    testresultspane.cpp \
    testresultmodel.cpp \
    testresultdelegate.cpp \
    testtreeitemdelegate.cpp \
    testsettings.cpp \
    testsettingspage.cpp \
    testnavigationwidget.cpp \
    testxmloutputreader.cpp \
    testsquishfilehandler.cpp \
    opensquishsuitesdialog.cpp \
    testsquishutils.cpp \
    testsquishtools.cpp

HEADERS += \
    squishsettings.h \
    squishsettingspage.h \
    testtreeview.h \
    testtreemodel.h \
    testtreeitem.h \
    testvisitor.h \
    testinfo.h \
    testcodeparser.h \
    autotestplugin.h \
    autotest_global.h \
    autotestconstants.h \
    testrunner.h \
    testconfiguration.h \
    testresult.h \
    testresultspane.h \
    testresultmodel.h \
    testresultdelegate.h \
    testtreeitemdelegate.h \
    testsettings.h \
    testsettingspage.h \
    testnavigationwidget.h \
    testxmloutputreader.h \
    testsquishfilehandler.h \
    opensquishsuitesdialog.h \
    testsquishutils.h \
    testsquishtools.h

RESOURCES += \
    autotest.qrc

FORMS += \
    squishsettingspage.ui \
    testsettingspage.ui \
    opensquishsuitesdialog.ui

equals(TEST, 1) {
    HEADERS += autotestunittests.h
    SOURCES += autotestunittests.cpp
    RESOURCES += autotestunittests.qrc
}
