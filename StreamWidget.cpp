#include "StreamWidget.h"
#include "ui_StreamWidget.h"
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QLabel>              // 修复：添加 QLabel 头文件
#include <QRegularExpression>  // 修复：添加正则头文件

StreamWidget::StreamWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StreamWidget)
{
    ui->setupUi(this);

    url[0] = "rtsp://192.168.9.81:554/11";
    url[1] = "rtsp://192.168.9.82:554/11";
    url[2] = "rtsp://192.168.9.83:554/11";

    xmlPt = QApplication::applicationDirPath() + "/camera_params.xml";

    // ==========================================
    //   第一路：绑定 label_1 (数组索引 0)
    // ==========================================
    m_capThreads[0] = new CaptureThread(&m_queues[0], this);
    m_procThreads[0] = new ProcessThread(&m_queues[0], this);

    ui->label_1->setScaledContents(true); // 让图片自适应 Label 大小

    // 连接信号
    connect(m_procThreads[0], &ProcessThread::frameReady, this, [this](QImage img){
        if(!img.isNull()) ui->label_1->setPixmap(QPixmap::fromImage(img));
    });

    // ==========================================
    //   第二路：绑定 label_2 (数组索引 1)
    // ==========================================
    m_capThreads[1] = new CaptureThread(&m_queues[1], this);
    m_procThreads[1] = new ProcessThread(&m_queues[1], this);

    ui->label_2->setScaledContents(true);

    connect(m_procThreads[1], &ProcessThread::frameReady, this, [this](QImage img){
        if(!img.isNull()) ui->label_2->setPixmap(QPixmap::fromImage(img));
    });

    // ==========================================
    //   第三路：绑定 label_3 (数组索引 2)
    // ==========================================
    m_capThreads[2] = new CaptureThread(&m_queues[2], this);
    m_procThreads[2] = new ProcessThread(&m_queues[2], this);

    ui->label_3->setScaledContents(true);

    connect(m_procThreads[2], &ProcessThread::frameReady, this, [this](QImage img){
        if(!img.isNull()) ui->label_3->setPixmap(QPixmap::fromImage(img));
    });

    // 延时加载配置，确保 Label 尺寸已初始化
    QTimer::singleShot(100, this, &StreamWidget::initConfigs);
}

StreamWidget::~StreamWidget() {
    for (int i = 0; i < 3; ++i) {
        if(m_capThreads[i]) m_capThreads[i]->stop();
        if(m_procThreads[i]) m_procThreads[i]->stop();
    }
    delete ui;
}

// --- XML 解析器 ---
bool StreamWidget::loadCameraParams(const QString& xmlPath, cv::Mat& K, cv::Mat& NewK, cv::Mat& D) {
    QFile file(xmlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open XML:" << xmlPath;
        return false;
    }

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        file.close();
        return false;
    }
    file.close();

    K = cv::Mat::eye(3, 3, CV_64F);
    NewK = cv::Mat::eye(3, 3, CV_64F);
    D = cv::Mat::zeros(1, 5, CV_64F);

    QDomElement root = doc.documentElement();
    QDomNode node = root.firstChild();

    while (!node.isNull()) {
        QDomElement e = node.toElement();
        if (!e.isNull()) {
            if (e.tagName() == "camera_matrix") {
                for(int i=0; i<9; i++) {
                    K.at<double>(i/3, i%3) = e.firstChildElement(QString("data%1").arg(i)).text().toDouble();
                }
            }
            else if (e.tagName() == "new_camera_matrix") {
                for(int i=0; i<9; i++) {
                    NewK.at<double>(i/3, i%3) = e.firstChildElement(QString("data%1").arg(i)).text().toDouble();
                }
            }
            else if (e.tagName() == "camera_distortion") {
                for(int i=0; i<5; i++) {
                    D.at<double>(0, i) = e.firstChildElement(QString("data%1").arg(i)).text().toDouble();
                }
            }
        }
        node = node.nextSibling();
    }
    return true;
}

// --- TXT 点位解析器 (已修复 Qt6 兼容性) ---
bool StreamWidget::loadCalibPoints(const QString& txtPath, std::vector<cv::Point2f>& points) {
    QFile file(txtPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open TXT:" << txtPath;
        return false;
    }

    QTextStream in(&file);
    points.clear();
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        // 修复：Qt6 使用 QRegularExpression 和 Qt::SkipEmptyParts
        QStringList parts = line.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);

        if (parts.size() >= 2) {
            points.push_back(cv::Point2f(parts[0].toFloat(), parts[1].toFloat()));
        }
    }
    file.close();
    return (points.size() == 4);
}

void StreamWidget::initConfigs() {
    // 1. 加载公共内参 (假设所有相机用同一个xml，如果不同请移入循环)
    cv::Mat K, NewK, D;
    if (!loadCameraParams(xmlPt, K, NewK, D)) {
        qDebug() << "Error: Could not load camera_params.xml";
        return;
    }

    // 2. 逐个配置 3 个通道
    for (int i = 0; i < 3; ++i) {
        // 加载 IPM 源点
        QString txtPath = QApplication::applicationDirPath() + QString("/calib_points_%1.txt").arg(i + 1);
        std::vector<cv::Point2f> srcPoints;
        if (!loadCalibPoints(txtPath, srcPoints)) {
            qDebug() << "Error: Could not load points from" << txtPath;
            continue;
        }

        // 1. 获取对应 Label 的指针
        QLabel* currentLabel = (i==0) ? ui->label_1 : (i==1 ? ui->label_2 : ui->label_3);

        // 2. 获取 Label 的实际尺寸
        int w = currentLabel ? currentLabel->width() : 400;
        int h = currentLabel ? currentLabel->height() : 600;
        if (w < 10) w = 400; // 兜底
        if (h < 10) h = 600;

        // 3. 设定目标点为控件的四个顶点 (对应 srcPoints 的顺序)
        // 假设 srcPoints 顺序是：左上 -> 右上 -> 右下 -> 左下
        std::vector<cv::Point2f> dstPoints;
        dstPoints.push_back(cv::Point2f(0, 0));       // Label 左上
        dstPoints.push_back(cv::Point2f(w, 0));       // Label 右上
        dstPoints.push_back(cv::Point2f(w, h));       // Label 右下
        dstPoints.push_back(cv::Point2f(0, h));       // Label 左下

        // 4. 计算 IPM 矩阵
        cv::Mat M_IPM = cv::getPerspectiveTransform(srcPoints, dstPoints);

        // 设置参数 (修复：传入 5 个参数)
        m_procThreads[i]->setParameters(K, D, M_IPM, cv::Size(1920, 1080), NewK, cv::Size(w, h));
        m_capThreads[i]->setUrl(url[i]);

        // 启动
        m_capThreads[i]->start();
        m_procThreads[i]->start();

        qDebug() << "Channel" << i+1 << "started.";
    }
}
