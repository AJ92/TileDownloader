#ifndef STICHWORKER_H
#define STICHWORKER_H

#include <QDebug>
#include <QStringList>
#include <QVariant>
#include <QDir>
#include <QHash>
#include <QVector2D>
#include "maputils.h"
#include <QImage>
#include <QPainter>

class StichWorker : public QObject
{
    Q_OBJECT

public:
    StichWorker( QObject* parent = 0);
    ~StichWorker();

public slots:
    void slotExecute( const QString& inputDir );

signals:
    void results( const QString& result );
    void status(const QString& status);

private:
    bool fileExists(QString path);
    void getFilenamesInDir(QString dir, QStringList &filenames);
    void fillTilesToLoadFromDisk();
    QString current_outputPath;
    QList<QVector2D> tiles_to_load;
    QHash<int, QHash<int, QString> > tiles_to_stich;
};

#endif // STICHWORKER_H
