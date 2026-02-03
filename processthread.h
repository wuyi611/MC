#ifndef PROCESSTHREAD_H
#define PROCESSTHREAD_H

#include <QThread>
#include <QImage>
#include <opencv2/opencv.hpp>
#include "FrameQueue.h"

class ProcessThread : public QThread {
    Q_OBJECT
public:
    explicit ProcessThread(FrameQueue* queue, QObject *parent = nullptr);

    // 【修改】增加 outputSize 参数
    void setParameters(const cv::Mat& K, const cv::Mat& D, const cv::Mat& IPM,
                       cv::Size inputSize, const cv::Mat& NewK, cv::Size outputSize);

    void stop();

signals:
    void frameReady(QImage image);

protected:
    void run() override;

private:
    FrameQueue* m_queue;
    bool m_stopRequested;
    bool m_paramsDirty;

    cv::Mat m_K, m_D, m_IPM, m_NewK;
    cv::Size m_inputSize;  // 原 m_size，改名以区分
    cv::Size m_outputSize; // 【新增】用于存储控件大小
    cv::Mat m_map1, m_map2;
};

#endif
