#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkRequest>
#include <QNetworkProxy>
#include <QUrl>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStateMachine>
#include <QPointer>

namespace NetworkPrivate
{
  class HttpReqState;
}

namespace Network
{
  class Reply;

  class Manager : public QObject
  {
    Q_OBJECT

    QNetworkProxy _proxy;

  public:
    explicit Manager(QObject *parent = 0) : QObject(parent) { }

    void setProxy(const QNetworkProxy &proxy) { _proxy = proxy; }

    Reply *get(const QNetworkRequest &req, bool recover = false);
    Reply *get(const QUrl &url, bool recover = false);
    Reply *get(const QUrl &url, const QString &fileName);

    Reply *post(const QNetworkRequest &req, bool recover = false);
    Reply *post(const QUrl &url, bool recover = false);
    Reply *post(const QUrl &url, const QString &fileName);

    QNetworkProxy proxy() const { return _proxy; }
  };

  class Reply : public QObject
  {
    Q_OBJECT

    friend class Manager;
    friend class NetworkPrivate::HttpReqState;

  public:
    static const int MAX_TRIES = 5;
    static const int TIMEOUT = 30000;
    static const int BUF_SIZE = 16384;

  private:
    QNetworkAccessManager::Operation _operation;
    bool _isFirstRequest;
    bool _isRecoverable;
    int _totalSize;
    int _currentSize;
    QNetworkRequest _req;
    bool _isRecoverActive;
    bool _usingFile;
    QNetworkProxy _proxy;
    QByteArray _buffer;
    QFile _file;
    QStateMachine *_stateMachine;
    QPointer< QNetworkReply > _trackedReply;

    explicit Reply(QNetworkAccessManager::Operation operation,
                   const QNetworkRequest &req, bool recover,
                   const QNetworkProxy &proxy);
    explicit Reply(QNetworkAccessManager::Operation operation,
                   const QNetworkRequest &req, const QString &fileName,
                   const QNetworkProxy &proxy);

    void buildAndStartStateMachine();

    void trackReply(QNetworkReply *reply) { _trackedReply = reply; }

  private slots:
    void updatePointers();
    void updateInfo(int size, bool recoverable);
    void appendMoreData(const QByteArray &data);
    void signalOk();
    void signalError();

  public:
    bool isRecoverActive() const { return _isRecoverActive; }
    QNetworkRequest request() const { return _req; }

    int currentSize() const { return _currentSize; }
    int totalSize() const { return _totalSize; }
    QNetworkAccessManager::Operation operation() const { return _operation; }

  signals:
    void finished(bool ok, const QByteArray &data);
    void progress(qint64 bytesReceived, qint64 bytesTotal);

  public slots:
    void abort();
  };
}

#endif // NETWORKMANAGER_H
