#include "ProcessThread.h"

ProcessThread::ProcessThread(FrameQueue* queue, QObject *parent)
    : QThread(parent), m_queue(queue), m_stopRequested(false), m_paramsDirty(false) {}

// 【修改】接收 outputSize
void ProcessThread::setParameters(const cv::Mat& K, const cv::Mat& D, const cv::Mat& IPM,
                                  cv::Size inputSize, const cv::Mat& NewK, cv::Size outputSize) {
    m_K = K.clone();
    m_D = D.clone();
    m_IPM = IPM.clone();
    m_NewK = NewK.clone();
    m_inputSize = inputSize;   // 输入视频分辨率 (如 1920x1080)
    m_outputSize = outputSize; // 【新增】输出控件分辨率 (如 label宽x高)
    m_paramsDirty = true;
}

void ProcessThread::stop() {
    m_stopRequested = true;
    requestInterruption();
    wait();
}

void ProcessThread::run() {
    m_stopRequested = false;
    cv::Mat frame, undistorted, warped, rgb;
    int frameCounter = 0; // 1. 定义计数器

    while (!m_stopRequested && !isInterruptionRequested()) {
        if (!m_queue->pop(frame)) continue;

        // frameCounter++;
        // if (frameCounter % 3 != 0) {
        //     continue; // 直接跳过，不进行 remap 和 warp
        // }


        // 1. 初始化 (只运行一次)
        if (m_paramsDirty && !m_K.empty()) {
            cv::Mat P = m_NewK.empty() ? m_K : m_NewK;
            // 注意：这里用 m_inputSize (1920x1080)，因为是对原始视频去畸变
            cv::initUndistortRectifyMap(m_K, m_D, cv::Mat(), P, m_inputSize, CV_32FC1, m_map1, m_map2);
            m_paramsDirty = false;
        }

        // 2. 畸变矫正
        if (!m_map1.empty()) cv::remap(frame, undistorted, m_map1, m_map2, cv::INTER_LINEAR);
        else undistorted = frame;

        // 3. 逆透视变换 (IPM)
        if (!m_IPM.empty()) {
            // 【关键修改】使用 m_outputSize 作为变换后的画布大小
            // 这样图像就会铺满整个 Label，且四个点对应 Label 的四个角
            cv::warpPerspective(undistorted, warped, m_IPM, m_outputSize);
        }
        else {
            // 如果没有IPM，为了适应控件，稍微resize一下（可选）
            if (undistorted.size() != m_outputSize)
                cv::resize(undistorted, warped, m_outputSize);
            else
                warped = undistorted;
        }

        // 4. 转 RGB 并发送
        if (!warped.empty()) {
            cv::cvtColor(warped, rgb, cv::COLOR_BGR2RGB);
            // 此时 rgb 的大小就是 m_outputSize (Label的大小)
            QImage qimg(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
            emit frameReady(qimg.copy());
        }
    }
}
