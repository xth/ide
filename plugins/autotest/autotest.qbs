import qbs

QtcPlugin {
    name: "AutoTest"

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "CPlusPlus" }
    Depends { name: "LicenseChecker" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmakeProjectManager" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "Utils" }

    Depends {
        name: "QtSupport"
        condition: project.testsEnabled
    }

    Depends {
        name: "Qt.test"
        condition: project.testsEnabled
    }

    Depends { name: "Qt.widgets" }

    files: [
        "autotest.qrc",
        "autotest_global.h",
        "autotestconstants.h",
        "autotestplugin.cpp",
        "autotestplugin.h",
        "squishsettings.cpp",
        "squishsettings.h",
        "squishsettingspage.cpp",
        "squishsettingspage.h",
        "squishsettingspage.ui",
        "testcodeparser.cpp",
        "testcodeparser.h",
        "testconfiguration.cpp",
        "testconfiguration.h",
        "testinfo.cpp",
        "testinfo.h",
        "testnavigationwidget.cpp",
        "testnavigationwidget.h",
        "testresult.cpp",
        "testresult.h",
        "testresultdelegate.cpp",
        "testresultdelegate.h",
        "testresultmodel.cpp",
        "testresultmodel.h",
        "testresultspane.cpp",
        "testresultspane.h",
        "testrunner.cpp",
        "testrunner.h",
        "testsettings.cpp",
        "testsettings.h",
        "testsettingspage.cpp",
        "testsettingspage.h",
        "testsettingspage.ui",
        "testtreeitem.cpp",
        "testtreeitem.h",
        "testtreeitemdelegate.cpp",
        "testtreeitemdelegate.h",
        "testtreemodel.cpp",
        "testtreemodel.h",
        "testtreeview.cpp",
        "testtreeview.h",
        "testvisitor.cpp",
        "testvisitor.h",
        "testxmloutputreader.cpp",
        "testxmloutputreader.h",
        "testsquishfilehandler.cpp",
        "testsquishfilehandler.h",
        "opensquishsuitesdialog.cpp",
        "opensquishsuitesdialog.h",
        "opensquishsuitesdialog.ui",
        "testsquishutils.cpp",
        "testsquishutils.h",
        "testsquishtools.cpp",
        "testsquishtools.h",
        "squishxmloutputhandler.cpp",
        "squishxmloutputhandler.h"
    ]

    Group {
        name: "Test sources"
        condition: project.testsEnabled
        files: [
            "autotestunittests.cpp",
            "autotestunittests.h",
            "autotestunittests.qrc",
        ]
    }

    Group {
        name: "Auto Test Wizard"
        prefix: "../../shared/autotest/"
        files: [
            "*"
        ]
        fileTags: []
        qbs.install: true
        qbs.installDir: project.ide_data_path + "/templates/wizards/autotest"
    }
}
