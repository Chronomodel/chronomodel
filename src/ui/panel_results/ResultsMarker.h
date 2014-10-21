#ifndef ResultsMarker_H
#define ResultsMarker_H

#include <QWidget>


class ResultsMarker: public QWidget
{
    Q_OBJECT
public:
    ResultsMarker(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ResultsMarker();
    
    int thickness() const;
    
protected:
    void paintEvent(QPaintEvent* e);
};

#endif
