#include "transfersftp.h"

// curl
#include <curl/curl.h>

#include <fstream>

// test
#include <QDebug>
#include <QThread>
#include <QFileInfo>

typedef struct
{
    std::string type;
    std::string remote;
    std::string local;
    float rate;
    TransferSftp *ts;
}ProcessData;

static size_t read_data_func(void *buffer, size_t size, size_t nmemb, void *data)
{
    std::fstream *fp = static_cast<std::fstream*>(data);
    if (nullptr == fp) return 0;

    if (!fp->is_open()) return 0;
    fp->read((char*)buffer, size * nmemb);
    return fp->gcount();
}

static size_t write_data_func(void *ptr, size_t size, size_t nmemb, void *data)
{
    std::fstream *fp = static_cast<std::fstream*>(data);
    if (nullptr == fp) return 0;

    fp->write((char*)ptr, size * nmemb);
    return size * nmemb;
}

static size_t debug_data_func(CURL *handle, curl_infotype type, char *data, size_t size, void *clientp)
{
    Q_UNUSED(handle);Q_UNUSED(type);Q_UNUSED(size);Q_UNUSED(clientp);
    qDebug() << "debug message " << data;
    return 0;
}

static size_t progress_data_func(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    ProcessData *processData = (ProcessData*)(clientp);
    if (nullptr == processData) return 0;

    if (processData->type == "upload")
    {
        uint16_t rate = (ultotal == 0) ? 0 : (ulnow * 10000 / ultotal);
        if ((rate - processData->rate >= 5) || ((rate == 10000) && (rate != processData->rate)))
        {
            processData->rate = rate;
            emit processData->ts->sgl_file_upload_process(processData->local.data(), rate);
        }
    }
    else if (processData->type == "download")
    {
        uint16_t rate = (dltotal == 0) ? 0 : (dlnow * 10000 / dltotal);
        if ((rate - processData->rate >= 5) || ((rate == 10000) && (rate != processData->rate)))
        {
            processData->rate = rate;
            emit processData->ts->sgl_file_download_process(processData->local.data(), rate);
        }
    }
    return 0;
}

TransferSftp::TransferSftp(QObject *parent) : QObject{parent}
{

}

TransferSftp::~TransferSftp()
{
    curl_global_cleanup();
}

TransferSftp *TransferSftp::getInstance()
{
    static TransferSftp inst;
    return &inst;
}

void TransferSftp::upload(const QString &local, const QString &remote)
{
    std::lock_guard<std::mutex> lock(mMutexQueue);
    if (!mInstanceInit) init();

    TransMission mission = { true, remote, local };
    mTransMissionQueue.enqueue(mission);
    mCVQueue.notify_one();
}

void TransferSftp::download(const QString &remote, const QString &local)
{
    std::lock_guard<std::mutex> lock(mMutexQueue);
    if (!mInstanceInit) init();

    TransMission mission = { false, remote, local };
    mTransMissionQueue.enqueue(mission);
    mCVQueue.notify_one();
}

void TransferSftp::init()
{
    mInstanceInit = true;
    for (int i = 0; i < mMaxCount; i++)
    {
        auto func = std::bind(&TransferSftp::transfer, this);
        std::thread th(func);
        th.detach();
    }
}

void TransferSftp::transfer()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *handle = curl_easy_init();

    while (nullptr == handle)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        handle = curl_easy_init();
    }

    while (true)
    {
        std::unique_lock<std::mutex> lock(mMutexQueue);
        mCVQueue.wait(lock, [this]{ return mTransMissionQueue.size() > 0; });
        TransMission mission = mTransMissionQueue.dequeue();
        lock.unlock();

        if (nullptr == handle)
        {
            qDebug() << "thread is over because fail init curl";
            return;
        }

        // clear opt
        curl_easy_reset(handle);
        QString url = QString("sftp://%1%2").arg("192.168.3.132", mission.remote);
        curl_easy_setopt(handle, CURLOPT_URL, url.toLatin1().data());
        curl_easy_setopt(handle, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PASSWORD);
        curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(handle, CURLOPT_PROTOCOLS, CURLPROTO_SFTP);
        curl_easy_setopt(handle, CURLOPT_USERNAME, "ftp12138");
        curl_easy_setopt(handle, CURLOPT_PASSWORD, "123456");

        curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, progress_data_func);

        curl_easy_setopt(handle, CURLOPT_DEBUGFUNCTION, debug_data_func);

        // 0 去掉调试信息, 1 添加调试信息
        curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L);

        ProcessData processData = { "", mission.remote.toStdString(), mission.local.toStdString(), 0.0, this};

        std::fstream fp;
        if (mission.isUpload)
        {
            fp = std::fstream(mission.local.toStdString(), std::ios::binary | std::ios::in);
            if (!fp.is_open())
            {
                qDebug() << "can not open local file " << mission.local;
                continue;
            }

            uint64_t fileSize = QFileInfo(mission.local).size();

            processData.type = "upload";
            curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &processData);

            curl_easy_setopt(handle, CURLOPT_READFUNCTION, read_data_func);
            curl_easy_setopt(handle, CURLOPT_READDATA, &fp);
            curl_easy_setopt(handle, CURLOPT_FTP_CREATE_MISSING_DIRS, 1);
            curl_easy_setopt(handle, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(handle, CURLOPT_INFILESIZE, (curl_off_t)fileSize);
        }
        else
        {
            fp = std::fstream(mission.local.toStdString(), std::ios::binary | std::ios::out);
            if (!fp.is_open())
            {
                qDebug() << "file create failed " << mission.local;
                continue;
            }

            processData.type = "download";
            curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &processData);

            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data_func);
            curl_easy_setopt(handle, CURLOPT_WRITEDATA, &fp);
        }

        CURLcode code = curl_easy_perform(handle);
        if(CURLE_OK != code)
        {
            qDebug() << "curl told us "<< code << url << (fp.is_open());
        }
        fp.close();
        if ((CURLE_OK != code) && (!mission.isUpload)) QFile::remove(mission.local);
    }
}
