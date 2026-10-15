/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
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
    void addTab(const QString& name, qsizetype identifier);
    void addTab(QWidget* wid, const QString& name);
    void setTab(const qsizetype &index, bool notify);
    void setTabId(const qsizetype id, bool notify);
    void setFont(const QFont &font);
    void setTabHeight(const int& h) {mTabHeight = h;}

    void setTabVisible(const qsizetype& i , const bool visible) {
        mTabVisible[i] = visible;
        updateLayout();
    }
    
    qsizetype currentIndex() const;
    int currentId() const;

    QString currentName() const;

    // getter
    QWidget* getWidget(const qsizetype& i);
    QWidget* getCurrentWidget();
    QRect widgetRect();

    QRect minimalGeometry() const;
    int minimalHeight() const;
    int minimalWidth() const;
    virtual QSize sizeHint() const;
    virtual int height() const {return mTabHeight;};

    int tabHeight() const { return mTabHeight;}

signals:
    void tabClicked(const qsizetype& index);

public slots:
    void showWidget(const qsizetype& i);

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
    QList<qsizetype> mTabIds;
    QList<bool> mTabVisible;
    qsizetype mCurrentIndex;
};

#endif
