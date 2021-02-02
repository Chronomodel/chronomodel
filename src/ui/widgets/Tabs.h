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

#ifndef TABS_H
#define TABS_H

#include <QWidget>

class Tabs: public QWidget
{
    Q_OBJECT
public:
    Tabs(QWidget* parent = nullptr);
    ~Tabs();

    // setter
    void addTab(const QString& name);
    void addTab(const QString& name, int identifier);
    void addTab(QWidget* wid, const QString& name);
    void setTab(const int &index, bool notify);
    void setTabId(const int identifier, bool notify);
    void setFont(const QFont &font);
    void setTabHeight(const int &h) {mTabHeight = h;}

    void setTabVisible(const int &i , const bool visible) {
        mTabVisible[i] = visible;
        updateLayout();
    }
    
    int currentIndex() const;
    int currentId() const;

    // getter
    QWidget* getWidget(const int &i);
    QWidget* getCurrentWidget();
    QRect widgetRect();

    QRect minimalGeometry() const;
    int minimalHeight() const;
    int minimalWidth() const;

    int tabHeight() const { return mTabHeight;}

signals:
    void tabClicked(const int &index);

public slots:
    void showWidget(const int &i);

protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void updateLayout();

private:
    int mTabHeight;
    QStringList mTabNames;
    QList<QRectF> mTabRects;
    QList<QWidget*> mTabWidgets;
    QList<int> mTabIds;
    QList<bool> mTabVisible;
    int mCurrentIndex;
};

#endif
