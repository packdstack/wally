#ifndef NETWORKMANAGER_P_H
#define NETWORKMANAGER_P_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QIODevice>
#include <QEvent>
#include <QState>
#include <QFinalState>
#include <QAbstractTransition>
#include <QSignalMapper>
#include <QByteArray>

#ifdef NMDEBUG
  #include <QtDebug>
#endif

namespace Network
{
  class Reply;
}

namespace NetworkPrivate
{
  class OKEvent : public QEvent
  {
    friend class HttpReqState;
    friend class NewTryState;
    friend class CheckDataState;

  public:
    static const int Type = QEvent::User + 1;

  private:
    explicit OKEvent() : QEvent(QEvent::Type(Type)) { }
  };

  class ErrorEvent : public QEvent
  {
    friend class HttpReqState;
    friend class NewTryState;
    friend class CheckDataState;

  public:
    static const int Type = QEvent::User + 2;

  private:
    explicit ErrorEvent() : QEvent(QEvent::Type(Type)) { }
  };

  class ReqWRecEvent : public QEvent
  {
    friend class InitialState;

  public:
    static const int Type = QEvent::User + 3;

  private:
    explicit ReqWRecEvent() : QEvent(QEvent::Type(Type)) { }
  };

  class ReqWORecEvent : public QEvent
  {
    friend class InitialState;

  public:
    static const int Type = QEvent::User + 4;

  private:
    explicit ReqWORecEvent() : QEvent(QEvent::Type(Type)) { }
  };

  class Transition : public QAbstractTransition
  {
    Q_OBJECT

  protected:
    explicit Transition(QAbstractState *target, QState *sourceState = 0) :
      QAbstractTransition(sourceState) { setTargetState(target); }

  #ifdef NMDEBUG
    void onTransition(QEvent *event)
      { qDebug() << "entered state " << metaObject()->className() << ", cause of event " << event->type(); }
  #else
    void onTransition(QEvent *) { }
  #endif
  };

  class StartWRecTransition : public Transition
  {
    Q_OBJECT

    friend class Network::Reply;

    explicit StartWRecTransition(QAbstractState *target, QState *sourceState = 0) :
      Transition(target,sourceState) { }

  protected:
    bool eventTest(QEvent *event) { return (event->type() == ReqWRecEvent::Type); }
  };

  class StartWORecTransition : public Transition
  {
    Q_OBJECT

    friend class Network::Reply;

    explicit StartWORecTransition(QAbstractState *target, QState *sourceState = 0) :
      Transition(target,sourceState) { }

  protected:
    bool eventTest(QEvent *event) { return (event->type() == ReqWORecEvent::Type); }
  };

  class OKTransition : public Transition
  {
    Q_OBJECT

    friend class Network::Reply;

    explicit OKTransition(QAbstractState *target, QState *sourceState = 0) :
      Transition(target,sourceState) { }

  protected:
    bool eventTest(QEvent *event) { return (event->type() == OKEvent::Type); }
  };

  class ErrorTransition : public Transition
  {
    Q_OBJECT

    friend class Network::Reply;

    explicit ErrorTransition(QAbstractState *target, QState *sourceState = 0) :
      Transition(target,sourceState) { }

  protected:
    bool eventTest(QEvent *event) { return (event->type() == ErrorEvent::Type); }
  };

  class OneMoreTryTransition : public Transition
  {
    Q_OBJECT

    friend class Network::Reply;

    explicit OneMoreTryTransition(QAbstractState *target, QState *sourceState = 0) :
      Transition(target,sourceState) { }

  protected:
    bool eventTest(QEvent *event) { return (event->type() == OKEvent::Type); }
  };

  class State : public QState
  {
    Q_OBJECT

  protected:

  #ifdef NMDEBUG
    void onEntry(QEvent *event)
      { qDebug() << "entered state " << metaObject()->className() << ", cause of event " << event->type(); }
    void onExit(QEvent *event)
      { qDebug() << "exited state " << metaObject()->className() << ", cause of event " << event->type(); }
  #else
    void onEntry(QEvent *) { }
    void onExit(QEvent *) { }
  #endif

  public:
    State(QState *parent = 0) : QState(parent) { }
    State(ChildMode childMode, QState *parent = 0) : QState(childMode,parent) { }
  };

  class FinalState : public QFinalState
  {
    Q_OBJECT

  protected:

  #ifdef NMDEBUG
    void onEntry(QEvent *event)
      { qDebug() << "entered final state " << metaObject()->className() << ", cause of event " << event->type(); }
    void onExit(QEvent *event)
      { qDebug() << "exited final state " << metaObject()->className() << ", cause of event " << event->type(); }
  #else
    void onEntry(QEvent *) { }
    void onExit(QEvent *) { }
  #endif

    FinalState(QState *parent = 0) : QFinalState(parent) { }
  };

  class OKState : public FinalState
  {
    Q_OBJECT

    friend class Network::Reply;

    OKState(QState *parent = 0) : FinalState(parent) { }
  };

  class ErrorState : public FinalState
  {
    Q_OBJECT

    friend class Network::Reply;

    ErrorState(QState *parent = 0) : FinalState(parent) { }
  };

  class InitialState : public State
  {
    Q_OBJECT

    friend class Network::Reply;

    bool _recover;

    explicit InitialState(bool recover, QState *parent = 0) :
      State(parent), _recover(recover) { }

  protected:
    void onEntry(QEvent *event);
  };

  class NewTryState : public State
  {
    Q_OBJECT

    friend class Network::Reply;

    int _currentTry;

    explicit NewTryState(QState *parent = 0) :
      State(parent), _currentTry(0) { }

  protected:
    void onEntry(QEvent *event);
  };

  class NetworkAccessManager : public QNetworkAccessManager
  {
    Q_OBJECT

    QSignalMapper *_signalMapper;

  private slots:
    void abortReply(QObject *object);
    void handleSslErrors(QNetworkReply *reply, const QList< QSslError > &);

  protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0);

  public:
    NetworkAccessManager(QObject *parent = 0);
  };

  class HttpReqState : public State
  {
    Q_OBJECT

    Network::Reply *_networkReply;
    NetworkAccessManager *_manager;
    QNetworkRequest _request;

    QNetworkReply *createReply(QNetworkAccessManager *manager,
                               const QNetworkRequest &request);

  private slots:
    void processReply(QNetworkReply *reply);
    void processReplyData();

  protected:
    explicit HttpReqState(Network::Reply *reply,
                          const QNetworkProxy &proxy,
                          const QNetworkRequest &request,
                          QState *parent = 0);

    virtual QNetworkReply *doCreateReply(QNetworkAccessManager *manager,
                                         const QNetworkRequest &request) = 0;

    void onEntry(QEvent *event);

    virtual void doProcessReply(QNetworkReply *reply) = 0;
    virtual void doProcessReplyData(const QByteArray &data) = 0;

    Network::Reply *networkReply() { return _networkReply; }
  };

  class HeadReqState : public HttpReqState
  {
    Q_OBJECT

    friend class Network::Reply;

    explicit HeadReqState(Network::Reply *reply,
                          const QNetworkProxy &proxy,
                          const QNetworkRequest &request,
                          QState *parent = 0) :
      HttpReqState(reply,proxy,request,parent) { }

  protected:
    QNetworkReply *doCreateReply(QNetworkAccessManager *manager,
                                 const QNetworkRequest &request);
    void doProcessReply(QNetworkReply *reply);
    void doProcessReplyData(const QByteArray &) { }

  signals:
    void headerInfoAvailable(int size, bool recoverable);
  };

  class GetReqState : public HttpReqState
  {
    Q_OBJECT

    friend class Network::Reply;

    explicit GetReqState(Network::Reply *reply,
                         const QNetworkProxy &proxy,
                         const QNetworkRequest &request,
                         QState *parent = 0) :
      HttpReqState(reply,proxy,request,parent) { }

  protected:
    QNetworkReply *doCreateReply(QNetworkAccessManager *manager,
                                 const QNetworkRequest &request);
    void doProcessReply(QNetworkReply *) { }
    void doProcessReplyData(const QByteArray &data);

  signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void dataAvailable(const QByteArray &data);
    void newRequest();
  };

  class CheckDataState : public State
  {
    Q_OBJECT

    friend class Network::Reply;

    Network::Reply *_reply;

    explicit CheckDataState(Network::Reply *reply, QState *parent = 0) :
      State(parent), _reply(reply) { }

  protected:
    void onEntry(QEvent *event);
  };

}

#endif // NETWORKMANAGER_P_H
