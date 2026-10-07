/*
 * This file is part of the DSView project.
 * DSView is based on PulseView.
 *
 * Copyright (C) 2024 DreamSourceLab <support@dreamsourcelab.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef XSPINBOX_H
#define XSPINBOX_H

#include <QSpinBox>
#include <QWidget>

// Spin boxes that display and parse numbers the way the rest of DSView formats
// them: ASCII digits and '.' as the decimal point independent of system locale.
class XSpinBox : public QSpinBox
{
public:
    XSpinBox(QWidget *parent = nullptr);
    ~XSpinBox();

protected:
    QValidator::State validate(QString &text, int &pos) const override;
};

class XDoubleSpinBox : public QDoubleSpinBox
{
public:
    XDoubleSpinBox(QWidget *parent = nullptr);
    ~XDoubleSpinBox();

protected:
    QValidator::State validate(QString &text, int &pos) const override;
};

#endif