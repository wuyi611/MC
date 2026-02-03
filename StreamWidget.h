#ifndef STREAMWIDGET_H
#define STREAMWIDGET_H

#include <QWidget>
#include <QtXml>
#include <QFile>
#include <QTextStream>
#include <opencv2/opencv.hpp>
#include "CaptureThread.h"
#include "ProcessThread.h"
#include "FrameQueue.h"

namespace Ui { class StreamWidget; }

class StreamWidget : public QWidget {
    Q_OBJECT

public:
    explicit StreamWidget(QWidget *parent = nullptr);
    ~StreamWidget();

private:
    Ui::StreamWidget *ui;

    // 3路独立流水线数组
    FrameQueue      m_queues[3];
    CaptureThread* m_capThreads[3];
    ProcessThread* m_procThreads[3];
    QString url[3];
    QString xmlPt;

    // 初始化逻辑
    void initConfigs();

    // 解析辅助函数
    bool loadCameraParams(const QString& xmlPath, cv::Mat& K, cv::Mat& NewK, cv::Mat& D);
    bool loadCalibPoints(const QString& txtPath, std::vector<cv::Point2f>& points);
};

#endif // STREAMWIDGET_H
