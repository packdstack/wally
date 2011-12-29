#include <QVariant>
#include <QTimer>
#include <QListIterator>
#include <QPair>
#include <QStringList>

#include "networkmanager.h"
#include "networkmanager_p.h"

using namespace Network;
using namespace NetworkPrivate;

Reply *Manager::get(const QNetworkRequest &req, bool recover)
{
  return new Reply(QNetworkAccessManager::GetOperation,req,recover,_proxy);
}

Reply *Manager::get(const QUrl &url, bool recover)
{
  return new Reply(QNetworkAccessManager::GetOperation,QNetworkRequest(url),recover,_proxy);
}

Reply *Manager::get(const QUrl &url, const QString &fileName)
{
  return new Reply(QNetworkAccessManager::GetOperation,QNetworkRequest(url),fileName,_proxy);
}

Reply *Manager::post(const QNetworkRequest &req, bool recover)
{
  return new Reply(QNetworkAccessManager::PostOperation,req,recover,_proxy);
}

Reply *Manager::post(const QUrl &url, bool recover)
{
  return new Reply(QNetworkAccessManager::PostOperation,QNetworkRequest(url),recover,_proxy);
}

Reply *Manager::post(const QUrl &url, const QString &fileName)
{
  return new Reply(QNetworkAccessManager::PostOperation,QNetworkRequest(url),fileName,_proxy);
}

Reply::Reply(QNetworkAccessManager::Operation operation,
             const QNetworkRequest &req, const QString &fileName,
             const QNetworkProxy &proxy) :
  QObject(),
  _operation(operation),
  _isFirstRequest(true),
  _isRecoverable(false),
  _totalSize(0),
  _currentSize(0),
  _req(req),
  _isRecoverActive(true),
  _usingFile(true),
  _proxy(proxy),
  _file(fileName),
  _stateMachine(new QStateMachine(this))
{
  _file.open(QIODevice::WriteOnly);

  buildAndStartStateMachine();
}

Reply::Reply(QNetworkAccessManager::Operation operation,
             const QNetworkRequest &req, bool recover,
             const QNetworkProxy &proxy) :
  QObject(),
  _operation(operation),
  _isFirstRequest(true),
  _isRecoverable(false),
  _totalSize(0),
  _currentSize(0),
  _req(req),
  _isRecoverActive(recover),
  _usingFile(false),
  _proxy(proxy),
  _stateMachine(new QStateMachine(this))
{
  buildAndStartStateMachine();
}

void Reply::buildAndStartStateMachine()
{
  QState *initialState = new InitialState(_isRecoverActive,_stateMachine);
  QState *getReqState = new GetReqState(this,_proxy,_req,_stateMachine);
  QState *headReqState = new HeadReqState(this,_proxy,_req,_stateMachine);
  QState *headNewTryState = new NewTryState(_stateMachine);
  QState *getNewTryState = new NewTryState(_stateMachine);
  QState *checkDataState = new CheckDataState(this,_stateMachine);
  QFinalState *okState = new OKState(_stateMachine);
  QFinalState *errorState = new ErrorState(_stateMachine);

  connect(headReqState,SIGNAL(headerInfoAvailable(int,bool)),
          this,SLOT(updateInfo(int,bool)));
  connect(getReqState,SIGNAL(dataAvailable(const QByteArray &)),
          this,SLOT(appendMoreData(const QByteArray &)));
  connect(getReqState,SIGNAL(newRequest()),
          this,SLOT(updatePointers()));
  connect(okState,SIGNAL(entered()),
          this,SLOT(signalOk()));
  connect(errorState,SIGNAL(entered()),
          this,SLOT(signalError()));

  initialState->addTransition(new StartWRecTransition(headReqState));
  initialState->addTransition(new StartWORecTransition(getReqState));

  headReqState->addTransition(new OKTransition(getReqState));
  headReqState->addTransition(new ErrorTransition(headNewTryState));

  headNewTryState->addTransition(new OneMoreTryTransition(headReqState));
  headNewTryState->addTransition(new ErrorTransition(errorState));

  getReqState->addTransition(new OKTransition(checkDataState));
  getReqState->addTransition(new ErrorTransition(getNewTryState));

  getNewTryState->addTransition(new OneMoreTryTransition(getReqState));
  getNewTryState->addTransition(new ErrorTransition(errorState));

  checkDataState->addTransition(new OKTransition(okState));
  checkDataState->addTransition(new ErrorTransition(getNewTryState));

  _stateMachine->setInitialState(initialState);
  _stateMachine->start();
}

void Reply::updatePointers()
{
  if (!_isRecoverable && !_isFirstRequest)
  {
    _currentSize = 0;

    if (_usingFile)
    {
      _file.close();
      _file.open(QIODevice::WriteOnly);
    }
    else
      _buffer.clear();
  }

  _isFirstRequest = false;
}

void Reply::updateInfo(int size, bool recoverable)
{
  _totalSize = size;
  _isRecoverable = recoverable;
  if (!_usingFile)
    _buffer.reserve(_totalSize);
}

void Reply::appendMoreData(const QByteArray &data)
{
  if (_usingFile)
    _file.write(data);
  else
    _buffer.append(data);

  _currentSize += data.size();
  emit progress(_currentSize,_totalSize);
}

void Reply::abort()
{
  _stateMachine->stop();
  if (!_trackedReply.isNull())
    _trackedReply->abort();
  signalError();
}

void Reply::signalOk()
{
  if (_usingFile)
    _file.close();
  emit finished(true,_buffer);
}

void Reply::signalError()
{
  if (_usingFile)
  {
    _file.close();
    _file.remove();
  }
  emit finished(false,QByteArray());
}

NetworkAccessManager::NetworkAccessManager(QObject *parent) :
  QNetworkAccessManager(parent),
  _signalMapper(new QSignalMapper(this))
{
  connect(_signalMapper,SIGNAL(mapped(QObject *)),
          this,SLOT(abortReply(QObject *)));
}

void NetworkAccessManager::abortReply(QObject *object)
{
  QNetworkReply *reply = qobject_cast< QNetworkReply * >(object);

  if (reply)
    reply->abort();
}

QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
  QNetworkReply *reply = QNetworkAccessManager::createRequest(op,req,outgoingData);
  QTimer *timer = new QTimer(this);

  _signalMapper->setMapping(timer,reply);

  timer->setInterval(Reply::TIMEOUT);
  timer->setSingleShot(true);
  connect(timer,SIGNAL(timeout()),_signalMapper,SLOT(map()));

  reply->setReadBufferSize(Reply::BUF_SIZE);

  connect(reply,SIGNAL(downloadProgress(qint64,qint64)),timer,SLOT(start()));
  connect(reply,SIGNAL(metaDataChanged()),timer,SLOT(start()));

  connect(reply,SIGNAL(error(QNetworkReply::NetworkError)),timer,SLOT(stop()));
  connect(reply,SIGNAL(aboutToClose()),timer,SLOT(stop()));
  connect(reply,SIGNAL(finished()),timer,SLOT(stop()));

  reply->open(QIODevice::ReadOnly);
  QTimer::singleShot(0,timer,SLOT(start()));

  return reply;
}

void InitialState::onEntry(QEvent *event)
{
  State::onEntry(event);

  if (_recover)
    machine()->postEvent(new ReqWRecEvent);
  else
    machine()->postEvent(new ReqWORecEvent);
}

HttpReqState::HttpReqState(Network::Reply *reply,
                           const QNetworkProxy &proxy,
                           const QNetworkRequest &request,
                           QState *parent) :
  State(parent),
  _networkReply(reply),
  _manager(new NetworkAccessManager(this)),
  _request(request)
{
  _manager->setProxy(proxy);
  connect(_manager,SIGNAL(finished(QNetworkReply *)),this,SLOT(processReply(QNetworkReply *)));
}

void HttpReqState::onEntry(QEvent *event)
{
  State::onEntry(event);

  createReply(_manager,_request);
}

QNetworkReply *HttpReqState::createReply(QNetworkAccessManager *manager,
                                         const QNetworkRequest &request)
{
  QNetworkReply *reply = doCreateReply(manager,request);

  networkReply()->trackReply(reply);
  connect(reply,SIGNAL(readyRead()),this,SLOT(processReplyData()));

  return reply;
}

void HttpReqState::processReplyData()
{
  QNetworkReply *reply = qobject_cast< QNetworkReply * >(sender());

  if (reply)
    doProcessReplyData(reply->readAll());
}

void HttpReqState::processReply(QNetworkReply *reply)
{
  reply->deleteLater();

  if (reply->error() == QNetworkReply::NoError)
  {
    if (!reply->attribute(QNetworkRequest::RedirectionTargetAttribute).isNull())
      createReply(reply->manager(),
                  QNetworkRequest(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl()));
    else
    {
      doProcessReply(reply);
      machine()->postEvent(new OKEvent);
    }
  }
  else
  {
    doProcessReply(reply);
    if (machine()->isRunning())
      machine()->postEvent(new ErrorEvent);
  }
}

QNetworkReply *HeadReqState::doCreateReply(QNetworkAccessManager *manager,
                                           const QNetworkRequest &request)
{
  QNetworkRequest req(request);

  req.setRawHeader("Connection","keep-alive");
  req.setRawHeader("Accept-Ranges","bytes");
  req.setRawHeader("Accept","*/*");

  return manager->head(req);
}

void HeadReqState::doProcessReply(QNetworkReply *reply)
{
  if (reply->error() == QNetworkReply::NoError)
  {
    QVariant size(reply->header(QNetworkRequest::ContentLengthHeader));

    if (!size.isNull())
      emit headerInfoAvailable(size.toInt(),
                               reply->hasRawHeader("Accept-Ranges") &&
                               QString(reply->rawHeader("Accept-Ranges")).compare("none",Qt::CaseInsensitive));
  }
}

void NewTryState::onEntry(QEvent *event)
{
  State::onEntry(event);

  if (++_currentTry == Reply::MAX_TRIES)
    machine()->postEvent(new ErrorEvent);
  else
    machine()->postEvent(new OKEvent);
}

QNetworkReply *GetReqState::doCreateReply(QNetworkAccessManager *manager,
                                          const QNetworkRequest &request)
{
  QNetworkRequest req(request);

  if (networkReply()->totalSize() && networkReply()->currentSize())
    req.setRawHeader("Range",QString("bytes=%1-%2").arg(networkReply()->currentSize())
                                                   .arg(networkReply()->totalSize()).toAscii());

  emit newRequest();

  switch (networkReply()->operation())
  {
    case QNetworkAccessManager::GetOperation:
      return manager->get(req);

    case QNetworkAccessManager::PostOperation:
      {
        QStringList data;
        QListIterator< QPair< QByteArray,QByteArray > > item(request.url().encodedQueryItems());

        while (item.hasNext())
        {
          QPair< QByteArray,QByteArray > i(item.next());

          data << QString("%1=%2").arg(i.first.constData()).arg(i.second.constData());
        }

        req.setUrl(req.url().toString(QUrl::RemoveQuery));
        return manager->post(req,data.join("&").toAscii());
      }

    default:
      return 0;
  }
}

void GetReqState::doProcessReplyData(const QByteArray &data)
{
  emit dataAvailable(data);
}

void CheckDataState::onEntry(QEvent *event)
{
  State::onEntry(event);

  if ((!_reply->totalSize()) ||
      (_reply->currentSize() == _reply->totalSize()))
    machine()->postEvent(new OKEvent);
  else
    machine()->postEvent(new ErrorEvent);
}
