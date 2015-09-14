#include "tiledownloader.h"
#include "ui_tiledownloader.h"
#include <QMessageBox>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QFileDialog>
#include <QTreeView>
#include "maputils.h"
#include <QDebug>
#include "filedownloader.h"
#include <QFileInfo>
#include <QPainter>


TileDownloader::TileDownloader(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TileDownloader)
{
    ui->setupUi(this);

    //////////////////////////////////////////////////
    //DOWNLOADING STUFF

    //keep 32 images in ram and store once full or done
    buffer_size = 1024;

    connect(ui->pushButtonCancel,SIGNAL(clicked()),this,SLOT(cancelDownload()));
    connect(ui->pushButtonBrowse,SIGNAL(clicked()),this,SLOT(setOutputPath()));
    connect(ui->pushButtonDownload,SIGNAL(clicked()),this,SLOT(downloadTiles()));
    connect(ui->pushButtonStich,SIGNAL(clicked()),this,SLOT(stichDownloadedTiles()));

    fdl_1 = new FileDownloader(this);
    connect(fdl_1, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_1()));

    fdl_2 = new FileDownloader(this);
    connect(fdl_2, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_2()));

    fdl_3 = new FileDownloader(this);
    connect(fdl_3, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_3()));

    fdl_4 = new FileDownloader(this);
    connect(fdl_4, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_4()));


    //////////////////////////////////////////////////
    //STICHING STUFF
    m_stichthread = new Stichthread();

    // Worker thread
    connect( m_stichthread, SIGNAL( progress(const QString&) ),
             this, SLOT( threadStatus(const QString&) ) );
    /*
    connect( m_stichthread, SIGNAL( ready(bool) ),
             goButton, SLOT( setEnabled(bool) ) );
    */
    connect( m_stichthread, SIGNAL( results( const QString& ) ),
             this, SLOT( threadDone( const QString& ) ) );

    // Launch worker thread
    m_stichthread->start();

}

TileDownloader::~TileDownloader()
{
    m_stichthread->quit();
    m_stichthread->wait();
    delete m_stichthread;
    delete ui;
}

void TileDownloader::setProgress(int done, int from){
    ui->progressBar->setValue((int)(100.0 * ((double)done/(double)from)));
    ui->labelProgress->setText(QString::number(done) + "/" + QString::number(from));
}

void TileDownloader::setOutputPath(){
    QFileDialog *fd = new QFileDialog;
    fd->setFileMode(QFileDialog::Directory);
    fd->setOption(QFileDialog::ShowDirsOnly);
    int result = fd->exec();
    QString directory;
    if (result)
    {
        directory = fd->selectedFiles()[0];
        ui->lineEditOutputPath->setText(directory);
    }
}

void TileDownloader::downloadTiles(){
    //get all the values:
    double longLeft = ui->doubleSpinBoxLeftLong->value();
    double longRight = ui->doubleSpinBoxRightLong->value();
    double latTop = ui->doubleSpinBoxTopLat->value();
    double latBottom = ui->doubleSpinBoxBottomLat->value();

    int level = ui->spinBoxLevel->value();

    QString outputPath = ui->lineEditOutputPath->text();

    if(longLeft >= longRight){
        QMessageBox msgBox;
        msgBox.setWindowTitle("Tile Downloader");
        msgBox.setText("Error.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setInformativeText("Left Long is bigger than Right Long (or equal).");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        int ret = msgBox.exec();
        return;
    }

    if(latBottom >= latTop){
        QMessageBox msgBox;
        msgBox.setWindowTitle("Tile Downloader");
        msgBox.setText("Error.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setInformativeText("Bottom Lat is bigger than Top Lat (or equal).");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        int ret = msgBox.exec();
        return;
    }

    //try to create the output path if not existing
    QDir dir;
    if(!dir.mkpath(outputPath)){
        QMessageBox msgBox;
        msgBox.setWindowTitle("Tile Downloader");
        msgBox.setText("Error.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setInformativeText("Couldn't create output path.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        int ret = msgBox.exec();
        return;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //work it
    tiles_to_load.clear();
    current_tile_index = 0;

    current_tile_index_1 = 0;
    current_tile_index_2 = 0;
    current_tile_index_3 = 0;
    current_tile_index_4 = 0;


    current_outputPath = outputPath;
    current_level = level;

    image_buffer.clear();
    path_buffer.clear();

    //get topLeft tile
    QVector2D topLeftTile;
    {
        int pixelX;
        int pixelY;
        int tileX;
        int tileY;
        MapUtils::LatLongToPixelXY(latTop,longLeft,level,pixelX,pixelY);
        MapUtils::PixelXYToTileXY(pixelX,pixelY,tileX,tileY);
        topLeftTile.setX(tileX);
        topLeftTile.setY(tileY);
    }

    //get bottomRight tile
    QVector2D bottomRightTile;
    {
        int pixelX;
        int pixelY;
        int tileX;
        int tileY;
        MapUtils::LatLongToPixelXY(latBottom,longRight,level,pixelX,pixelY);
        MapUtils::PixelXYToTileXY(pixelX,pixelY,tileX,tileY);
        bottomRightTile.setX(tileX);
        bottomRightTile.setY(tileY);
    }


    qDebug() << topLeftTile;
    qDebug() << MapUtils::TileXYToQuadKey(topLeftTile.x(),topLeftTile.y(),level);

    qDebug() << bottomRightTile;
    qDebug() << MapUtils::TileXYToQuadKey(bottomRightTile.x(),bottomRightTile.y(),level);


    //fill the list with all TILES TO LOAD
    for(int x_coord = topLeftTile.x(); x_coord <= bottomRightTile.x(); x_coord++){
        for(int y_coord = topLeftTile.y(); y_coord <= bottomRightTile.y(); y_coord++){
            tiles_to_load.append(QVector2D(x_coord,y_coord));
        }
    }

    qDebug() << tiles_to_load.size();
    startImgDownloader_1();
    startImgDownloader_2();
    startImgDownloader_3();
    startImgDownloader_4();

}

void TileDownloader::startImgDownloader_1(){
    setProgress(current_tile_index,tiles_to_load.size());

    if(image_buffer.size() >= buffer_size){
        writeImagesToDisk();
    }

    disconnect(fdl_1, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_1()));
    if(current_tile_index < tiles_to_load.size()){
        current_tile_index_1 = current_tile_index;
        current_tile_index += 1;

        connect(fdl_1, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_1()));
        fdl_1->download(getnextUrl(current_tile_index_1));
    }
    else{
        downloadDone();
    }
}

void TileDownloader::startImgDownloader_2(){
    setProgress(current_tile_index,tiles_to_load.size());

    if(image_buffer.size() >= buffer_size){
        writeImagesToDisk();
    }

    disconnect(fdl_2, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_2()));
    if(current_tile_index < tiles_to_load.size()){
        current_tile_index_2 = current_tile_index;
        current_tile_index += 1;

        connect(fdl_2, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_2()));
        fdl_2->download(getnextUrl(current_tile_index_2));
    }
    else{
        downloadDone();
    }
}

void TileDownloader::startImgDownloader_3(){
    setProgress(current_tile_index,tiles_to_load.size());

    if(image_buffer.size() >= buffer_size){
        writeImagesToDisk();
    }

    disconnect(fdl_3, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_3()));
    if(current_tile_index < tiles_to_load.size()){
        current_tile_index_3 = current_tile_index;
        current_tile_index += 1;

        connect(fdl_3, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_3()));
        fdl_3->download(getnextUrl(current_tile_index_3));
    }
    else{
        downloadDone();
    }
}

void TileDownloader::startImgDownloader_4(){
    setProgress(current_tile_index,tiles_to_load.size());

    if(image_buffer.size() >= buffer_size){
        writeImagesToDisk();
    }

    disconnect(fdl_4, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_4()));
    if(current_tile_index < tiles_to_load.size()){
        current_tile_index_4 = current_tile_index;
        current_tile_index += 1;

        connect(fdl_4, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_4()));
        fdl_4->download(getnextUrl(current_tile_index_4));
    }
    else{
        downloadDone();
    }
}

QUrl TileDownloader::getnextUrl(int tileIndex){
    //build url...

    //bing has multiple servers to balance loads so lets use them...
    //random number between 0 and 3
    int randServer = rand()%(3-0 + 1) + 0;

    //https://t3.ssl.ak.tiles.virtualearth.net/tiles/a122100100223310303.jpeg?g=3675&n=z
    //https://t1.ssl.ak.tiles.virtualearth.net/tiles/a120212020102130102.jpeg?g=3986&n=z

    return QUrl("https://t" +
                  QString::number(randServer) +
                  ".ssl.ak.tiles.virtualearth.net/tiles/a" +
                  MapUtils::TileXYToQuadKey(tiles_to_load[tileIndex].x(),
                                            tiles_to_load[tileIndex].y(),
                                            current_level)+
                  ".jpeg?g=3986&n=z");
}

void TileDownloader::loadAndSaveImage_1(){
    QImage img;
    img.loadFromData(fdl_1->downloadedData());

    storeDownloadedDiskInBuffer(img,current_tile_index_1);

    //current_tile_index += 1;
    startImgDownloader_1();
}

void TileDownloader::loadAndSaveImage_2(){
    QImage img;
    img.loadFromData(fdl_2->downloadedData());

    storeDownloadedDiskInBuffer(img,current_tile_index_2);

    //current_tile_index += 1;
    startImgDownloader_2();
}

void TileDownloader::loadAndSaveImage_3(){
    QImage img;
    img.loadFromData(fdl_3->downloadedData());

    storeDownloadedDiskInBuffer(img,current_tile_index_3);

    //current_tile_index += 1;
    startImgDownloader_3();
}

void TileDownloader::loadAndSaveImage_4(){
    QImage img;
    img.loadFromData(fdl_4->downloadedData());

    storeDownloadedDiskInBuffer(img,current_tile_index_4);

    //current_tile_index += 1;
    startImgDownloader_4();
}

void TileDownloader::storeDownloadedDiskInBuffer(QImage &img, int tileIndex){
    ui->imgPreview->setPixmap(QPixmap::fromImage(img));
    image_buffer.append(img);
    path_buffer.append(current_outputPath + "/" +
                       QString::number(tiles_to_load[tileIndex].x()) + "-" +
                       QString::number(tiles_to_load[tileIndex].y()) + ".png");
}

void TileDownloader::downloadDone(){
    writeImagesToDisk();
    QApplication::beep();
    /*
    QMessageBox msgBox;
    msgBox.setWindowTitle("Tile Downloader");
    msgBox.setText("Done.");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setInformativeText("Downloaded all images!");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    int ret = msgBox.exec();
    */
}

void TileDownloader::cancelDownload(){
    disconnect(fdl_1, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_1()));
    disconnect(fdl_2, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_2()));
    disconnect(fdl_3, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_3()));
    disconnect(fdl_4, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage_4()));
    ui->progressBar->setValue(0);

    QMessageBox msgBox;
    msgBox.setWindowTitle("Tile Downloader");
    msgBox.setText("Canceled.");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setInformativeText("Canceled all jobs!");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

void TileDownloader::writeImagesToDisk(){
    for(int i = 0; i < image_buffer.size(); i++){
        image_buffer.at(i).save(path_buffer.at(i));
    }
    image_buffer.clear();
    path_buffer.clear();
}






//STICH


void TileDownloader::stichDownloadedTiles(){
    m_stichthread->execute( ui->lineEditOutputPath->text() );
}

void TileDownloader::threadStatus(const QString& message){
    ui->statusBar->showMessage( "Stiching: " + message, 3000);
}

void TileDownloader::threadDone(const QString &message){
    QMessageBox msgBox;
    msgBox.setWindowTitle("Tile Downloader");
    msgBox.setText("Stiching stopped");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setInformativeText(message);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}
