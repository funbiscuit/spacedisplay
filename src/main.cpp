
#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
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

    MainWindow mainWin;
    mainWin.show();
    return QApplication::exec();
}
