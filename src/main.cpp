#include "mainwindow.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("error-surface");
    QApplication::setApplicationVersion("1.0");
    
    qDebug() << "\nError Surface - Qt6 Native Edition\n";
    
    MainWindow window(7, 60, 5);  // defaults: 7 days, 60 min, 5 sec
    window.show();
    window.startCollections();
    
    return app.exec();
}
