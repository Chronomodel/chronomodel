#ifndef CALIBRATIONDRAWING_H
#define CALIBRATIONDRAWING_H

#include <QWidget>
#include <QLabel>
class GraphView;
class GraphViewRefAbstract;
class Marker;

class CalibrationDrawing : public QWidget
{
    Q_OBJECT
public:
    explicit CalibrationDrawing(QWidget *parent = nullptr);
    ~CalibrationDrawing();

    void setTitle(const QString& title) { mTitle->setText(title);}
    void setRefTitle(const QString& title) { mRefTitle->setText(title);}
    void setRefComment(const QString& comment) { mRefComment->setText(comment);}
    void addRefGraph(GraphViewRefAbstract* refGraph);

    void setCalibTitle(const QString& title) { mCalibTitle->setText(title);}
    void setCalibComment(const QString& comment) { mCalibComment->setText(comment);}
    void addCalibGraph(GraphView* calibGraph);

    void setFont(const QFont& font) { mFont = font; update();}
    void hideMarker();
    void showMarker();
    virtual QFont font() const {return mFont;}

private:
    QLabel* mTitle;
    QLabel* mRefTitle;
    QLabel* mRefComment;
    GraphViewRefAbstract* mRefGraphView;

    QLabel* mCalibTitle;
    QLabel* mCalibComment;
    GraphView* mCalibGraph;

    Marker* mMarkerX;
    Marker* mMarkerY;

    int mVerticalSpacer;
    QFont mFont;

protected:
    void paintEvent(QPaintEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();

signals:


public slots:
};

#endif // CALIBRATIONDRAWING_H
