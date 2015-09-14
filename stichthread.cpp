#include "stichthread.h"
#include <QDebug>
#include <QStringList>
#include <QVariant>
#include <QDir>
#include "stichworker.h"

Stichthread::Stichthread(QObject *parent)
    : QThread(parent)
{
}

Stichthread::~Stichthread()
{
    delete m_worker;
}

void Stichthread::execute( const QString& query )
{
    emit executefwd( query ); // forwards to the worker
}

void Stichthread::run()
{
    emit ready(false);
    emit progress( "Stichthread starting, one moment please..." );

    // Create worker object within the context of the new thread
    m_worker = new StichWorker();

    connect( this, SIGNAL( executefwd( const QString& ) ),
             m_worker, SLOT( slotExecute( const QString& ) ) );


    // Critical: register new type so that this signal can be
    // dispatched across thread boundaries by Qt using the event
    // system
    //qRegisterMetaType< QList<QSqlRecord> >( "QList<QSqlRecord>" );

    // forward final signal
    connect( m_worker, SIGNAL( results( const QString& ) ),
             this, SIGNAL( results( const QString& ) ) );


    connect( m_worker, SIGNAL( status( const QString& ) ),
             this, SIGNAL( progress( const QString& ) ) );

    emit progress( "Stichthread started ready to rock!." );
    emit ready(true);

    exec();  // our event loop
}
