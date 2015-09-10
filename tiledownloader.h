#ifndef TILEDOWNLOADER_H
#define TILEDOWNLOADER_H

#include <QMainWindow>
#include <QVector2D>
#include <QList>
#include <QImage>
#include <QHash>

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
    FileDownloader * fdl;

    QString current_outputPath;
    QList<QVector2D> tiles_to_load;
    int current_tile_index;
    int current_level;

    QList<QImage> image_buffer;
    QList<QString> path_buffer;
    int buffer_size;

private slots:
    void setProgress(int done, int from);
    void setOutputPath();

    //todo... send to a thread or so...
    void downloadTiles();
    void startImgDownloader();
    void cancelDownload();

    void loadAndSaveImage();
    void writeImagesToDisk();



//STICH
private:

    QHash<int, QHash<int, QString> > tiles_to_stich;

    bool fileExists(QString path);
    void getFilenamesInDir(QString dir, QStringList &filenames);
    void fillTilesToLoadFromDisk();

private slots:
    void stichDownloadedTiles();



};

#endif // TILEDOWNLOADER_H
