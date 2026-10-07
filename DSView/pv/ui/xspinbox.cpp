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

#include "xspinbox.h"

// A spin box uses its locale both to display and to parse the value, so both
// have to come from the same locale:
// - Some Windows locales make Qt draw digits as unrelated characters, e.g.
//   "Chinese (Simplified, Hong Kong SAR)" (upstream DSView issue #913).
// - Displaying "1.50" but parsing with a decimal-comma locale reads it as 150.
static QLocale number_locale()
{
    QLocale locale = QLocale::c();
    locale.setNumberOptions(QLocale::OmitGroupSeparator | QLocale::RejectGroupSeparator);
    return locale;
}

// Qt's spin box validator accepts group separators even with
// RejectGroupSeparator set, which would read a typed "2,25" as 225.
static bool has_group_separator(QString text, const QString &prefix, const QString &suffix)
{
    if (text.startsWith(prefix))
        text.remove(0, prefix.size());
    if (text.endsWith(suffix))
        text.chop(suffix.size());
    return text.contains(number_locale().groupSeparator());
}

XSpinBox::XSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    setLocale(number_locale());
}

XSpinBox::~XSpinBox()
{

}

QValidator::State XSpinBox::validate(QString &text, int &pos) const
{
    if (has_group_separator(text, prefix(), suffix()))
        return QValidator::Invalid;
    return QSpinBox::validate(text, pos);
}

//-------------------XDoubleSpinBox
XDoubleSpinBox::XDoubleSpinBox(QWidget *parent)
    : QDoubleSpinBox(parent)
{
    setLocale(number_locale());
}

XDoubleSpinBox::~XDoubleSpinBox()
{

}

QValidator::State XDoubleSpinBox::validate(QString &text, int &pos) const
{
    if (has_group_separator(text, prefix(), suffix()))
        return QValidator::Invalid;
    return QDoubleSpinBox::validate(text, pos);
}