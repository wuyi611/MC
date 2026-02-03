#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H

#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <opencv2/opencv.hpp>

class FrameQueue {
public:
    void push(const cv::Mat& frame) {
        QMutexLocker locker(&mutex);
        // 如果积压超过 3 帧，丢弃最旧的帧，防止延迟
        if (queue.size() > 3) {
            queue.dequeue();
        }
        queue.enqueue(frame);
        condition.wakeOne();
    }

    bool pop(cv::Mat& frame) {
        QMutexLocker locker(&mutex);
        if (queue.isEmpty()) {
            // 等待新帧，超时 100ms 返回 false
            if (!condition.wait(&mutex, 100)) return false;
        }
        if (!queue.isEmpty()) {
            frame = queue.dequeue();
            return true;
        }
        return false;
    }

    void clear() {
        QMutexLocker locker(&mutex);
        queue.clear();
    }

private:
    QQueue<cv::Mat> queue;
    QMutex mutex;
    QWaitCondition condition;
};

#endif // FRAMEQUEUE_H
