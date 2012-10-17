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

#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QStringList>

#include <math.h>
#include "thermalregenerationDB.h"

ValueMap::ValueMap()
{

}

ValueMap::ValueMap(QStringList listVals)
{
    //init Map
    foreach (QString name, listVals)
    (*this)[name] = new QVector<double>();
}

ValueMap::~ValueMap()
{
    foreach(QString key, this->keys())
    delete (*this)[key];
    
}



/** @brief Reads the data from Robert Sitzenfrei's GWHP database
  *
  *
  *
  *  I(m/m)   kf(m/s)    kfHzuV(-)    Q(m3/s    TempR(∞C)    BD(h)    Disp(m)     L(m)       B(m)     Bhyd(m)     v(m/s)
  *  ==============================================================================================================================
  *  0.000250   0.050000         10   0.000500          5       2000          1 144.150225  15.998216   1.600000   0.000013
  *
  */
ThermalRegenerationDB::ThermalRegenerationDB(std::string filename)
{

    listVals << "I" << "kf" << "kfHzuV" << "Q" << "TempR" << "B" << "BD" << "Disp" << "L" << "Bhyd" << "v";
    
    database = ValueMap(listVals);

    
    QFile db(QString::fromStdString(filename));
    
    if (!db.open(QIODevice::ReadOnly | QIODevice::Text))
        return;    
    
    QTextStream in(&db);
    bool startReading = false;
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList slist = line.split(QRegExp("\\s+"));
        if ((slist.size() > 0) && (!startReading)) {
            if (slist[0].contains("=")) {
                startReading = true;
                continue;
            }
        }
        if (!startReading)
            continue;
        
        if (slist.size() != listVals.size())
            continue;
        
        for (int i = 0; listVals.size(); i++)
            database[listVals[i]]->push_back(slist[i].toDouble());     
        
    }
    
    db.close();
    
}

DM::Component ThermalRegenerationDB::getThermalRegernationField(double I, double kf, double kfHzuV, double Q, double BD)
{
    
    DM::Component thermalReg;
    QStringList searchList;
    
    searchList << "I" << "kf" << "kfHzuV" << "Q" << "BD";
    QVector<double> values;
    values << I << kf << kfHzuV << Q << BD;
    
    ValueMap searchMap = database;
    
    for (int i = 0; i < searchList.size(); i++) {
        searchMap = this->interpolate(searchMap, searchList[i], values[i]);
    }
    
    
    if (searchMap.size() == 0)
        return thermalReg;
    
    foreach(QString name, listVals) {
        thermalReg.addAttribute(name.toStdString(), (*(searchMap[name]))[0]);
    }
    
    return thermalReg;
}

ValueMap ThermalRegenerationDB::interpolate(const ValueMap & m, QString index, double val) {
    ValueMap ress;
    if (m.size() == 0)
        return ress;
    //find min values
    double minvalue = fabs( (*(m[index]))[0]-val );
    double value = (*(m[index]))[0];
    for (int i = 0; i < m.size(); i++) {
        if ( minvalue > fabs((*(m[index]))[i]- val ) ){
            minvalue = fabs((*(m[index]))[i]-val);
            value = (*(m[index]))[i];

        }
    }
      for (int i = 0; i < m.size(); i++) {
          if ((*(m[index]))[i] == value) {
              foreach(QString name, listVals) {
                  ress[index]->push_back((*(m[name]))[i]);
              }
          }
      }
    
    return ress;
}

