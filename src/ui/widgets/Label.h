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

#ifndef LABEL_H
#define LABEL_H

#include <QLabel>

class Label: public QLabel
{
    Q_OBJECT
public:
    explicit Label(QWidget* parent = nullptr);
    explicit Label(const QString& text, QWidget* parent = nullptr);

    ~Label();

    void setLight();
    void setDark();

    void setPalette(QPalette &palette);
    void setBackground(QColor color);

    void setIsTitle(bool isTitle);
    void setAdjustText(bool ad = true);


private:
    void init();
    void adjustFont();
    bool mIsTitle;
    bool mAdjustText;
    QPalette mPalette;

protected:
    void resizeEvent(QResizeEvent* e);
    virtual void paintEvent(QPaintEvent*);
};

#endif
