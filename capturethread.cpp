#include "CaptureThread.h"
#include <QDebug> // <--- 必须包含这个

CaptureThread::CaptureThread(FrameQueue* queue, QObject *parent)
    : QThread(parent), m_queue(queue), m_stopRequested(false) {}

void CaptureThread::setUrl(const QString& url) {
    m_url = url;
    // 【调试】打印一下设置进去的 URL，看看这里是不是空的
    qDebug() << "[CaptureThread] Set URL:" << m_url;
}

void CaptureThread::stop() {
    m_stopRequested = true;
    requestInterruption();
    wait();
}

void CaptureThread::run() {
    // 【修复1】如果是空地址，直接返回，不要传给 OpenCV，否则必崩
    if (m_url.isEmpty()) {
        qDebug() << "[Error] CaptureThread started with EMPTY URL! Thread ID:" << QThread::currentThreadId();
        return;
    }

    qDebug() << "[CaptureThread] Opening RTSP:" << m_url; // 【调试】看看最终运行时的 URL

    m_stopRequested = false;
    cv::VideoCapture cap;

    // 【优化】降低 RTSP 延迟的参数 (FFmpeg后端有效)
    // cap.set(cv::CAP_PROP_BUFFERSIZE, 0);

    // 打开视频流
    cap.open(m_url.toStdString());

    if (!cap.isOpened()) {
        qDebug() << "[Error] Failed to open RTSP stream:" << m_url;
        return;
    }

    cv::Mat frame;
    while (!m_stopRequested && !isInterruptionRequested()) {
        if (cap.read(frame) && !frame.empty()) {
            m_queue->push(frame.clone());
        } else {
            // 读不到帧时稍微休眠，防止 CPU 100%
            msleep(10);

            // 如果断流，可以尝试重新打开 (简单的重连机制)
            if (!cap.isOpened()) {
                cap.open(m_url.toStdString());
                qDebug() << "Reconnecting...";
            }
        }
    }
    cap.release();
    qDebug() << "[CaptureThread] Thread stopped.";
}
