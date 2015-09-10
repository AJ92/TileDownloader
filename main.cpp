//#include <QtGui/QApplication>
#include "tiledownloader.h"
#include <QApplication>
//#include <QPlastiqueStyle>
//#include <QCleanlooksStyle>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //a.setStyle(new QPlastiqueStyle);

    TileDownloader w;
    w.show();
    
    return a.exec();
}
