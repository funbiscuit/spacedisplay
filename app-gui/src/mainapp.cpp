
#include "mainapp.h"

#include <QApplication>

#include "mainwindow.h"
#include "resources.h"

#include "customstyle.h"

int MainApp::run(int argc, char *argv[])
{
    QApplication::setStyle(new CustomStyle);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("com.github.funbiscuit");
    QCoreApplication::setApplicationName("Space Display");
    QCoreApplication::setApplicationVersion("1.0.0");
//    QCommandLineParser parser;
//    parser.setApplicationDescription(QCoreApplication::applicationName());
//    parser.addHelpOption();
//    parser.addVersionOption();
//    parser.addPositionalArgument("file", "The file to open.");
//    parser.process(app);


    using namespace ResourceBuilder;
    auto& r = Resources::get();

    QIcon appIcon(r.get_vector_pixmap(ResId::__ICONS_SVG_APPICON_SVG, 16));
    //add other sizes
    for(int sz=32;sz<=256;sz*=2)
        appIcon.addPixmap(r.get_vector_pixmap(ResId::__ICONS_SVG_APPICON_SVG, sz));

    QApplication::setWindowIcon(appIcon);

    MainWindow mainWin;
    mainWin.show();
    return QApplication::exec();
}

