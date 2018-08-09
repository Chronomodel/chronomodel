#ifndef ScrollCompressor_H
#define ScrollCompressor_H

#include <QWidget>


class ScrollCompressor: public QWidget
{
    Q_OBJECT
public:
    ScrollCompressor(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~ScrollCompressor();
    
    void setVertical(bool vertical);
    void setProp(const double& prop, bool sendNotification = false);
    double getProp() const;
    void showText(const QString& text, bool show);
    
    QSize sizeHint() const;
    
protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void updateProp(QMouseEvent* e);
    
signals:
    void valueChanged(double value);
    
private:
    double mProp;
    bool mIsDragging;
    bool mShowText;
    bool mIsVertical;
    QString mText;
};

#endif
