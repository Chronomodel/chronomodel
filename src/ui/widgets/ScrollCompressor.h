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

#ifndef SCROLLCOMPRESSOR_H
#define SCROLLCOMPRESSOR_H

#include <QWidget>

class ScrollCompressor: public QWidget
{
    Q_OBJECT
public:
    ScrollCompressor(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~ScrollCompressor();

    void setVertical(bool vertical);
    void setProp(const double& prop, bool sendNotification = false);
    double getProp() const;
    void showText(const QString& text, bool show);

    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void updateProp(QMouseEvent* e);
    void mouseDoubleClickEvent(QMouseEvent *e);

signals:
    void valueChanged(double value);

private:
    double mProp;
    bool mIsDragging;
    bool mShowText;
    bool mIsVertical;
    QString mText;
};

#endif
