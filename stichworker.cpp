#include "stichworker.h"

StichWorker::StichWorker( QObject* parent )
    : QObject( parent )
{


}

StichWorker::~StichWorker()
{
}

bool StichWorker::fileExists(QString path){
    QFileInfo checkFile(path);
    // check if file exists and if yes: Is it really a file and no directory?
    if (checkFile.exists() && checkFile.isFile()) {
        return true;
    } else {
        return false;
    }
}

void StichWorker::getFilenamesInDir(QString dir, QStringList &filenames){
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

void StichWorker::fillTilesToLoadFromDisk(){
    QString outputPath = current_outputPath;

    //try to create the output path if not existing
    QDir dir;
    if(!dir.mkpath(outputPath)){
        emit status("Input path non existing.");
        return;
    }

    tiles_to_load.clear();

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

void StichWorker::slotExecute( const QString& inputDir ){
    emit status("Stiching started...");
    emit status("Checking path for already existing images!");
    current_outputPath = inputDir;
    fillTilesToLoadFromDisk();
    if(tiles_to_load.size() < 1){
        emit status("No images found in output path.");
        emit status("Aborting stiching job...");
        return;
    }




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
    int newTileSizeX = 4096 * 2;
    int newTileSizeY = 4096 * 2;

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

    emit status("Stiching done.");
    emit results("done");
}
