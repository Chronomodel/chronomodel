#ifndef ScrollCompressor_H
#define ScrollCompressor_H

#include <QWidget>


class ScrollCompressor: public QWidget
{
    Q_OBJECT
public:
    ScrollCompressor(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ScrollCompressor();
    
    void setVertical(bool vertical);
    void setProp(const float& prop, bool sendNotification = false);
    void showText(const QString& text, bool show);
    
    QSize sizeHint() const;
    
protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void updateProp(QMouseEvent* e);
    
signals:
    void valueChanged(float value);
    
private:
    float mProp;
    bool mIsDragging;
    bool mShowText;
    bool mIsVertical;
    QString mText;
};

#endif
