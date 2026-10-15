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

#ifndef COLLAPSIBLE_H
#define COLLAPSIBLE_H

#include <QWidget>
#include <QPropertyAnimation>

class Collapsible;


class CollapsibleHeader: public QWidget
{
    Q_OBJECT
public:
    CollapsibleHeader(Collapsible* collapsible, Qt::WindowFlags flags = Qt::Widget);
    ~CollapsibleHeader();

    void setTitle(const QString& title);

protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);

private:
    Collapsible* mCollapsible;
    QString mTitle;
};


class Collapsible: public QWidget
{
    Q_OBJECT
public:
    Collapsible(const QString& title, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~Collapsible();

    void setWidget(QWidget* widget, int height);
    void setTitle(const QString& title);
    bool opened() const;

public slots:
    void open(bool open);

protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();

private slots:
    void sendCollapsing(const QVariant& variant);

signals:
    void collapsing(int h);

private:
    CollapsibleHeader* mHeader;
    QWidget* mWidget;
    int mHeaderHeight;
    int mWidgetHeight;
    bool mOpened;

    QPropertyAnimation* mAnimation;
};

#endif
