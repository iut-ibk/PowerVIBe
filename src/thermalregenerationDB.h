/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of PowerVIBe
 *
 * Copyright (C) 2012  Christian Urich
 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef __DynaMind_ToolBox__thermalregenerationDB__
#define __DynaMind_ToolBox__thermalregenerationDB__


#include <dm.h>

#include <QVector>
#include <QMap>
#include <QString>
#include <QStringList>

typedef QMap<QString, QVector<double> > value_map;
class ValueMap : public value_map
{
public:
	ValueMap();
	ValueMap(QStringList);


};

class ThermalRegenerationDB
{
private:
	ValueMap database;
	QStringList listVals;
	ValueMap interpolate(ValueMap m, QString index, double val);

public:
	ThermalRegenerationDB(std::string filename);
	DM::Component getThermalRegernationField(double I, double TempR,double kf, double kfHzuV, double Q, double BD, double Disp);



};

#endif /* defined(__DynaMind_ToolBox__thermalregenerationDB__) */
