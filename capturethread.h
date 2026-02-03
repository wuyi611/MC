#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include <QThread>
#include "FrameQueue.h"

class CaptureThread : public QThread {
    Q_OBJECT
public:
    explicit CaptureThread(FrameQueue* queue, QObject *parent = nullptr);
    void setUrl(const QString& url);
    void stop();

protected:
    void run() override;

private:
    FrameQueue* m_queue;
    QString m_url;
    bool m_stopRequested;
};

#endif
