#ifndef TRANSFERSFTP_H
#define TRANSFERSFTP_H

#include <QObject>
#include <mutex>
#include <QQueue>

typedef struct
{
    bool isUpload;
    QString remote;
    QString local;
}TransMission;

class TransferSftp : public QObject
{
    Q_OBJECT
private:
    explicit TransferSftp(QObject *parent = nullptr);
    ~TransferSftp();
    TransferSftp(const TransferSftp& t) = delete;

signals:
    void sgl_file_upload_process(const QString &local, uint16_t rate);
    void sgl_file_download_process(const QString &remote, uint16_t rate);

public:
    static TransferSftp *getInstance();
    void upload(const QString &local, const QString &remote);
    void download(const QString &remote, const QString &local);

private:
    void init();
    void transfer();

private:
    bool mInstanceInit = false;

    std::mutex mMutexQueue;
    std::condition_variable mCVQueue;
    QQueue<TransMission> mTransMissionQueue;

    uint16_t mMaxCount = 5;
};

#endif // TRANSFERSFTP_H
