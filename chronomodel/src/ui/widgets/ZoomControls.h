#ifndef ZoomControls_H
#define ZoomControls_H

#include <QWidget>

class QPushButton;


class ZoomControls: public QWidget
{
    Q_OBJECT
public:
    ZoomControls(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ZoomControls();
    
signals:
    void zoomIn();
    void zoomOut();
    void zoomDefault();
    
private:
    QPushButton* mZoomIn;
    QPushButton* mZoomOut;
    QPushButton* mZoomDefault;
};

#endif
