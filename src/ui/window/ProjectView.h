/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2020

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

#ifndef PROJECTVIEW_H
#define PROJECTVIEW_H

#include "MCMCLoopChrono.h"
#include "AppSettings.h"
#include "Project.h"
#include "Tabs.h"

#include <QWidget>

class QStackedWidget;
class QTextEdit;
class QTabWidget;
class QVBoxLayout;

class ModelView;
class ResultsView;
class Event;

class ProjectView: public QWidget
{
    Q_OBJECT
public:
    ProjectView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~ProjectView();

    void setScreenDefinition();
    void resizeEvent(QResizeEvent* e);

    void setProject(Project* project);
    
    void resetInterface();

    void readSettings();
    void writeSettings();
   // void setFont(const QFont &font);
    void newPeriod();

    void updateMultiCalibration();
    void eventsAreSelected();

public slots:
    void initResults(Model*);
    void updateResults();
    void updateProject();
    void showModel();
    void showResults();
    void showLogTab(const int &i);
    
    void showLog();
    void showHelp(bool show);

    void applySettings(Model* model);
    void applyFilesSettings(Model* model);
    void updateResultsLog(const QString& log);
    
    void toggleCurve(bool toggle);

private:
    QStackedWidget* mStack;
    ModelView* mModelView;
    ResultsView* mResultsView;

    QWidget* mLogView;
    QVBoxLayout* mLogLayout;
    Tabs* mLogTabs;
    QTextEdit* mLogModelEdit;
    QTextEdit* mLogInitEdit;
    QTextEdit* mLogAdaptEdit;
    QTextEdit* mLogResultsEdit;
};

#endif
