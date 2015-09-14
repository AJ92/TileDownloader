#ifndef STICHTHREAD_H
#define STICHTHREAD_H

#include <QList>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QString>

class StichWorker; // forw decl

class Stichthread : public QThread
{
  Q_OBJECT

  public:
    Stichthread(QObject *parent = 0);
    ~Stichthread();

    void execute( const QString& query );

  signals:
    void progress( const QString& msg );
    void ready(bool);
    void results( const QString& results );

  protected:
    void run();

  signals:
    void executefwd( const QString& inputDir );

  private:
     StichWorker* m_worker;
};

#endif // STICHTHREAD_H
