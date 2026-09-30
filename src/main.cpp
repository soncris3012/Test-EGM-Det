#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc,char **argv){QApplication app(argc,argv);app.setApplicationName("EGM-Det Benchmark Tool");app.setOrganizationName("Test EGM-Det");app.setStyle(QStyleFactory::create("Fusion"));MainWindow w;w.show();return app.exec();}
