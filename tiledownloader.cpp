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

    //keep 32 images in ram and store once full or done
    buffer_size = 128;

    connect(ui->pushButtonCancel,SIGNAL(clicked()),this,SLOT(cancelDownload()));
    connect(ui->pushButtonBrowse,SIGNAL(clicked()),this,SLOT(setOutputPath()));
    connect(ui->pushButtonDownload,SIGNAL(clicked()),this,SLOT(downloadTiles()));
    connect(ui->pushButtonStich,SIGNAL(clicked()),this,SLOT(stichDownloadedTiles()));

    fdl = new FileDownloader(this);
    connect(fdl, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage()));
}

TileDownloader::~TileDownloader()
{
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
    startImgDownloader();

}

void TileDownloader::startImgDownloader(){
    setProgress(current_tile_index,tiles_to_load.size());

    if(image_buffer.size() >= buffer_size){
        writeImagesToDisk();
    }

    disconnect(fdl, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage()));
    if(current_tile_index < tiles_to_load.size()){
        //build url...

        //bing has multiple servers to balance loads so lets use them...
        //random number between 0 and 3
        int randServer = rand()%(3-0 + 1) + 0;

        //https://t3.ssl.ak.tiles.virtualearth.net/tiles/a122100100223310303.jpeg?g=3675&n=z
        //https://t1.ssl.ak.tiles.virtualearth.net/tiles/a120212020102130102.jpeg?g=3986&n=z

        /*
        QUrl imageUrl("http://ak.dynamic.t" +
                      QString::number(randServer) +
                      ".tiles.virtualearth.net/comp/ch/" +
                      MapUtils::TileXYToQuadKey(tiles_to_load[current_tile_index].x(),
                                                tiles_to_load[current_tile_index].y(),
                                                current_level)+
                      "?mkt=eng-eng&it=A,G,L,LA&shading=hill&og=98&n=z");
                      */

        QUrl imageUrl("https://t" +
                      QString::number(randServer) +
                      ".ssl.ak.tiles.virtualearth.net/tiles/a" +
                      MapUtils::TileXYToQuadKey(tiles_to_load[current_tile_index].x(),
                                                tiles_to_load[current_tile_index].y(),
                                                current_level)+
                      ".jpeg?g=3986&n=z");
        fdl->download(imageUrl);
        connect(fdl, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage()));
    }
    else{
        writeImagesToDisk();
        QMessageBox msgBox;
        msgBox.setWindowTitle("Tile Downloader");
        msgBox.setText("Done.");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setInformativeText("Downloaded all images!");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        int ret = msgBox.exec();
    }
}

void TileDownloader::loadAndSaveImage(){
    QImage img;
    img.loadFromData(fdl->downloadedData());

    ui->imgPreview->setPixmap(QPixmap::fromImage(img));

    image_buffer.append(img);
    path_buffer.append(current_outputPath + "/" +
                       QString::number(tiles_to_load[current_tile_index].x()) + "-" +
                       QString::number(tiles_to_load[current_tile_index].y()) + ".png");

    current_tile_index += 1;
    startImgDownloader();
}

void TileDownloader::cancelDownload(){
    disconnect(fdl, SIGNAL (downloaded()), this, SLOT (loadAndSaveImage()));
    ui->progressBar->setValue(0);

    QMessageBox msgBox;
    msgBox.setWindowTitle("Tile Downloader");
    msgBox.setText("Canceled.");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setInformativeText("Canceled all jobs!");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    int ret = msgBox.exec();
}

void TileDownloader::writeImagesToDisk(){
    for(int i = 0; i < image_buffer.size(); i++){
        image_buffer.at(i).save(path_buffer.at(i));
    }
    image_buffer.clear();
    path_buffer.clear();
}






//STICH

bool TileDownloader::fileExists(QString path){
    QFileInfo checkFile(path);
    // check if file exists and if yes: Is it really a file and no directory?
    if (checkFile.exists() && checkFile.isFile()) {
        return true;
    } else {
        return false;
    }
}

void TileDownloader::getFilenamesInDir(QString dir, QStringList &filenames){
    QDir currentDir(dir);

    currentDir.setFilter(QDir::Files);
    QStringList entries = currentDir.entryList();

    for( QStringList::ConstIterator entry=entries.begin(); entry!=entries.end(); ++entry )
    {
        QString filename=*entry;
        if((filename != ".") && (filename != ".."))
        {
            filenames.append(filename);
        }
    }
}

void TileDownloader::fillTilesToLoadFromDisk(){
    QString outputPath = ui->lineEditOutputPath->text();

    //try to create the output path if not existing
    QDir dir;
    if(!dir.mkpath(outputPath)){
        QMessageBox msgBox;
        msgBox.setWindowTitle("Tile Downloader");
        msgBox.setText("Error.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setInformativeText("Input path non existing.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        int ret = msgBox.exec();
        return;
    }

    QStringList inputFileNames;
    getFilenamesInDir(outputPath, inputFileNames);

    for(int i = 0; i < inputFileNames.size(); i++){
        QString currentFileName = inputFileNames[i];
        if(currentFileName.contains("-") && currentFileName.contains(QString(".png"),Qt::CaseInsensitive)){
            QStringList split = currentFileName.split("-");
            if(split.size() < 2){
                continue;
            }
            QString tileX = split[0];

            QString split2 = split[1];
            QStringList split3 = split2.split(".png",QString::KeepEmptyParts,Qt::CaseInsensitive);
            if(split3.size() < 1){
                continue;
            }

            QString tileY = split3[0];

            //convert strings to integers
            bool success = false;
            int iTileX = tileX.toInt(&success);
            success = false;
            int iTileY = tileY.toInt(&success);

            qDebug() << "X: " << iTileX << "  Y: " << iTileY;

            tiles_to_load.append(QVector2D(iTileX,iTileY));
        }
    }
}

void TileDownloader::stichDownloadedTiles(){
    QMessageBox msgBox;
    msgBox.setWindowTitle("Tile Downloader");
    msgBox.setText("This can take a while.");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setInformativeText("Don't worry, lean back and wait till i am done!");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();

    //calculate size of final image...
    if(tiles_to_load.size() < 1){
        QMessageBox msgBox;
        msgBox.setWindowTitle("Tile Downloader");
        msgBox.setText("No images downloaded during this session.");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setInformativeText("Checking output path for already existing images!");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();

        fillTilesToLoadFromDisk();
    }

    if(tiles_to_load.size() < 1){
        QMessageBox msgBox;
        msgBox.setWindowTitle("Tile Downloader");
        msgBox.setText("No images found in output path.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setInformativeText("Aborting stiching job...");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    current_outputPath = ui->lineEditOutputPath->text();


    //calculate min max
    int min_x;
    int min_y;
    int max_x;
    int max_y;

    //fill first data
    min_x = tiles_to_load[0].x();
    min_y = tiles_to_load[0].y();
    max_x = tiles_to_load[0].x();
    max_y = tiles_to_load[0].y();

    for(int i = 0; i < tiles_to_load.size(); i++){
        min_x = MapUtils::min(tiles_to_load[i].x(),min_x);
        min_y = MapUtils::min(tiles_to_load[i].y(),min_y);
        max_x = MapUtils::max(tiles_to_load[i].x(),max_x);
        max_y = MapUtils::max(tiles_to_load[i].y(),max_y);
    }




    //fill the hashmap for better order and fast lookups
    for(int i = 0; i < tiles_to_load.size(); i++){
        QVector2D tile = tiles_to_load[i];
        tiles_to_stich[tile.x() - min_x][tile.y() - min_y] =
                QString::number(tile.x()) + "-" +
                QString::number(tile.y()) + ".png";
    }




    //pre calc some values
    int newTileSizeX = 4096 * 3;
    int newTileSizeY = 4096 * 3;

    int tilesToFitX = newTileSizeX / 256;
    int tilesToFitY = newTileSizeY / 256;

    int newImageCount = ceil(((float)((max_x - min_x) * 256) / (float)newTileSizeX)) *
                        ceil(((float)((max_y - min_y) * 256) / (float)newTileSizeY));

    int newTileCountX = ceil(((float)((max_x - min_x) * 256) / (float)newTileSizeX));
    int newTileCountY = ceil(((float)((max_y - min_y) * 256) / (float)newTileSizeY));

    qDebug() << "Final output should be: " << (max_x - min_x) * 256 << "  " << (max_y - min_y) * 256;
    qDebug() << "Tiles to create: " << newImageCount;


    int current_big_tile_x = 0;
    int current_big_tile_y = 0;
    int current_tile_x = 0;
    int current_tile_y = 0;

    bool working = true;

    QImage *newBigTile = new QImage(
                newTileSizeX,
                newTileSizeY,
                QImage::Format_ARGB32
                );

    qDebug() << "Tile Size should be: x: " << newTileSizeX << "  y: " << newTileSizeY;
    qDebug() << "Tile Size: " << newBigTile->size();

    QPainter * painter = new QPainter(newBigTile);

    while(working){

        if(tiles_to_stich.contains(current_tile_x)){
            if(tiles_to_stich[current_tile_x].contains(current_tile_y)){
                //we have a value, so lets try and load it and to write it onto the big tile
                QImage tile(current_outputPath + "/" +
                            tiles_to_stich[(current_big_tile_x * tilesToFitX) + current_tile_x]
                            [(current_big_tile_y * tilesToFitX) + current_tile_y]);
                if(!tile.isNull()){
                    painter->drawImage(current_tile_x * 256,
                                       current_tile_y * 256,
                                       tile);
                    qDebug() << "drawing";
                }
                else{
                    qDebug() << "image was null";
                }
            }
        }



        current_tile_x += 1;

        if(current_tile_x >= tilesToFitX){
            current_tile_x = 0;
            current_tile_y += 1;
        }
        if(current_tile_y >= tilesToFitY){

            newBigTile->save(current_outputPath + "/" + "F-" +
                             QString::number(current_big_tile_x) + "-" +
                             QString::number(current_big_tile_y) + ".png");

            qDebug() << "saving (y)";

            current_tile_x = 0;
            current_tile_y = 0;
            current_big_tile_y += 1;

            delete painter;
            delete newBigTile;
            newBigTile = new QImage(
                            newTileSizeX,
                            newTileSizeY,
                            QImage::Format_ARGB32
                            );
            painter = new QPainter(newBigTile);
        }
        if(current_big_tile_y >= newTileCountY){

            newBigTile->save(current_outputPath + "/" + "F-" +
                             QString::number(current_big_tile_x) + "-" +
                             QString::number(current_big_tile_y) + ".png");

            qDebug() << "saving (x)";

            current_big_tile_y = 0;
            current_big_tile_x += 1;

            delete painter;
            delete newBigTile;
            newBigTile = new QImage(
                            newTileSizeX,
                            newTileSizeY,
                            QImage::Format_ARGB32
                            );
            painter = new QPainter(newBigTile);
        }
        if(current_big_tile_x >= newTileCountX){
            working = false;
        }
    }

    /*
    QImage *finalImage = new QImage(
                2000,//(max_x - min_x) * 256,
                2000,//(max_y - min_y) * 256,
                QImage::Format_ARGB32
                );
    */

    //QPainter painter(finalImage);

    //painter.drawPicture(0,0,);


    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Tile Downloader");
        msgBox.setText("Stiching done.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setInformativeText("Image should be in the output folder by now.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
    }
}
