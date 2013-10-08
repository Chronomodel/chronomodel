#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>

class GraphView;


class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget *parent = 0);

signals:

private slots:
    void openDatationDialog();

private:
    GraphView* mGraphView;
};

#endif // MAINWIDGET_H
