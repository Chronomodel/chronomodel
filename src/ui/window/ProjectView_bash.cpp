/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

#include "MainWindow_bash.h"
#include "ProjectView_bash.h"
//#include "ModelView.h"
//#include "ResultsView.h"
#include "Painting.h"
#include "AppSettings.h"

// Constructor / Destructor / Init
ProjectView::ProjectView(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags)
{
    //setMouseTracking(true);
    setScreenDefinition();

    mTable = new QTableWidget(this);
    mTable->setColumnCount(1);

    mAddButton = new QPushButton ("Add File", this);
    mRemoveButton = new QPushButton ("Remove File", this);

    connect(mAddButton, &QPushButton::pressed, this, &ProjectView::tableAdd);
    connect(mRemoveButton, &QPushButton::pressed, this, &ProjectView::tableRemove);

    mTable->setFixedWidth(width()/2);
    mTable->setGeometry(QRect(width()/4, 50, width()/2, height()));

    auto x1 = mTable->pos().x();
    x1 = x1 /2 - 75;
    mAddButton->setGeometry(QRect(0, 0, 150, 0));
    mRemoveButton->setGeometry(QRect(10, mAddButton->pos().y()+mAddButton->height() + 10, 150, 0));

    mLog = new QTextEdit(this);
}

ProjectView::~ProjectView()
{

}

void ProjectView::setScreenDefinition()
{
    /* find screen definition */
    QScreen *screen;

  /*  int numScreen = QApplication::desktop()->screenNumber(this));

    if (numScreen>0) {
        screen = QApplication::screens().at(numScreen);

    } else {
        screen =  QGuiApplication::primaryScreen();
      //  numScreen = 0;
    }
    */
    screen =  QGuiApplication::primaryScreen();
    //qreal mm_per_cm = 10;

    qreal cm_per_in = 2.54;
 // look for screen definition
//        qDebug()<<"ProjectView::resizeEvent()"<< screen->name() <<" number="<<numScreen <<"logical Dots="<<screen->logicalDotsPerInch()<<"devicePixelRatio="<<screen->devicePixelRatio()<<"availableVirtualGeometry()="<<screen->availableVirtualGeometry()<<" unitX ="<<screen->logicalDotsPerInch() / cm_per_in;
//        qDebug()<<"ProjectView::resizeEvent()"<< numScreen << QApplication::desktop()->screenGeometry(numScreen) << QApplication::desktop()->availableGeometry(numScreen)<< QApplication::desktop()->width();
//        qDebug()<<"ProjectView::resizeEvent() screen setWidthUnit"<< screen->physicalDotsPerInchX() / cm_per_in;
//        qDebug()<<"ProjectView::resizeEvent() screen setHeigthUnit"<< screen->physicalDotsPerInchY() / cm_per_in;

            int unitX = int(screen->logicalDotsPerInch() / cm_per_in);
            AppSettings::setWidthUnit( unitX);

            int unitY = int(screen->logicalDotsPerInch() / cm_per_in);
            AppSettings::setHeigthUnit( unitY);



   /*const int logTabHusefull (height() - mLogTabs->tabHeight() - AppSettings::heigthUnit());

    mLogModelEdit->resize( width() - AppSettings::widthUnit(), logTabHusefull );
    mLogMCMCEdit->resize( width() - AppSettings::widthUnit(), logTabHusefull );
    mLogResultsEdit->resize( width() - AppSettings::widthUnit() , logTabHusefull );

*/

    //int unitY = int(screen->physicalDotsPerInchY() / cm_per_in);
    AppSettings::setHeigthUnit(unitY);
}

void ProjectView::resizeEvent(QResizeEvent* e)
{
    (void)e;
    mTable->setFixedWidth(width()/3);
    mTable->setColumnWidth(0, width()/3);
    mTable->setGeometry(QRect(width()/3, 20, width()/3, height() - 20));

    auto x1 = mTable->pos().x();
    x1 = x1 /2 - 75;
    mAddButton->setGeometry(QRect(x1, 50, 150, 50));

    mRemoveButton->setGeometry(QRect(x1, mAddButton->pos().y() + mAddButton->height() + 10, 150, 50));

    x1 = mTable->pos().x() + mTable->width();

    mLog->setGeometry(QRect(x1 + 10, mTable->pos().y(), width() - x1 - 20, mTable->height()));

}

void ProjectView::tableAdd()
{
    const QString currentPath = QDir::homePath();
    QString path = QFileDialog::getOpenFileName(this,
                                                      tr("Open File"),
                                                      currentPath,
                                                      tr("Chronomodel Project (*.chr)"));

    if (!path.isEmpty()) {
        mTable->setRowCount(mTable->rowCount()+1);

        QTableWidgetItem *newItem = new QTableWidgetItem(path);
        mTable->setItem(mTable->rowCount()-1 , 0, newItem );
    }
}

void ProjectView::tableRemove()
{
    auto selecI = mTable->selectedItems();
    for (auto i=selecI.rbegin() ; i != selecI.rend(); i++)
        mTable->removeRow((*i)->row());

}
void ProjectView::applyFilesSettings(Model* model)
{
    (void) model;
    // Rebuild all calibration curve

  //  QJsonObject state = mProject->state();//mModelView->getProject()->state();
  //  ProjectSettings s = ProjectSettings::fromJson(state.value(STATE_SETTINGS).toObject());
   // bool calibrate = mModelView->findCalibrateMissing();
   // if (calibrate)
 //       calibrateAll(s);

   // applySettings(model);
}

