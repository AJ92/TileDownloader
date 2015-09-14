#ifndef TILEDOWNLOADER_H
#define TILEDOWNLOADER_H

#include <QMainWindow>
#include <QVector2D>
#include <QList>
#include <QImage>
#include <QHash>
#include <QUrl>

#include "stichthread.h"

namespace Ui {
class TileDownloader;
}

class FileDownloader;

class TileDownloader : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit TileDownloader(QWidget *parent = 0);
    ~TileDownloader();
    
//DOWNLOAD
private:
    Ui::TileDownloader *ui;

    FileDownloader * fdl_1;
    FileDownloader * fdl_2;
    FileDownloader * fdl_3;
    FileDownloader * fdl_4;

    QString current_outputPath;
    QList<QVector2D> tiles_to_load;

    int current_tile_index;

    int current_tile_index_1;
    int current_tile_index_2;
    int current_tile_index_3;
    int current_tile_index_4;


    int current_level;

    QList<QImage> image_buffer;
    QList<QString> path_buffer;
    int buffer_size;

private slots:
    void setProgress(int done, int from);
    void setOutputPath();

    //todo... send to a thread or so...
    void downloadTiles();
    void cancelDownload();

    void startImgDownloader_1();
    void startImgDownloader_2();
    void startImgDownloader_3();
    void startImgDownloader_4();


    void loadAndSaveImage_1();
    void loadAndSaveImage_2();
    void loadAndSaveImage_3();
    void loadAndSaveImage_4();

    void writeImagesToDisk();

    void storeDownloadedDiskInBuffer(QImage &img, int tileIndex);

    QUrl getnextUrl(int tileIndex);
    void downloadDone();


//STICH
private:
    Stichthread * m_stichthread;

private slots:
    void stichDownloadedTiles();
    void threadStatus(const QString& message);
    void threadDone(const QString &message);



};

#endif // TILEDOWNLOADER_H
