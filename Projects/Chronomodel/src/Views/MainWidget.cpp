#include "PluginInterface.h"
#include "MainWidget.h"
#include "GraphView.h"
#include <QVBoxLayout>
#include <QAction>
#include <QtGui>

MainWidget::MainWidget(QWidget *parent) :
    QWidget(parent)
{
    mGraphView = new GraphView();
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(mGraphView);
    this->setLayout(layout);

    this->setBaseSize(600, 500);
}

void MainWidget::openDatationDialog()
{
    QAction* action = qobject_cast<QAction*>(sender());
    PluginInterface* iPlugin = qobject_cast<PluginInterface*>(action->parent());
    QVector<double> density = iPlugin->getDensity();
   // qDebug() << density;
    mGraphView->setValues(density);
}
