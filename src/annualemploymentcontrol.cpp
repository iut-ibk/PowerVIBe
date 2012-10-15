/**
 * @file
 * @author  Chrisitan Urich <christian.urich@gmail.com>
 * @version 1.0
 * @section LICENSE
 *
 * This file is part of DynaMind
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

#include "annualemploymentcontrol.h"
#include <QtSql>
#include <QUuid>

#include <sstream>

DM_DECLARE_NODE_NAME(AnnualEmploymentControl, PowerVIBe)
AnnualEmploymentControl::AnnualEmploymentControl()
{
    this->startYear = 2011;
    this->endYear = 2020;
    this->growthRate = 1.01;

    this->addParameter("StartYear", DM::INT, &startYear);
    this->addParameter("EndYear", DM::INT, &endYear);
    this->addParameter("GrowthRate", DM::DOUBLE, &this->growthRate);

    city = DM::View("CITY", DM::FACE, DM::MODIFY);
    city.getAttribute("JOBS");


    std::vector<DM::View> data;
    data.push_back(city);
    this->addData("City", data);



}

void AnnualEmploymentControl::run()
{


    DM::System * sys = this->getData("City");
    int numberOfJobs;

    std::vector<std::string> uuids = sys->getUUIDsOfComponentsInView(city);
    DM::Component * city = sys->getComponent(uuids[0]);
    numberOfJobs = (int)city->getAttribute("JOBS")->getDouble();

    QSqlDatabase db;
    db = QSqlDatabase::addDatabase("QMYSQL", QUuid::createUuid().toString());
    db.setHostName("127.0.0.1");
    db.setUserName("urbansim");
    db.setPassword("urbansim");

    bool ok = db.open();
    if( ok == false) {
        Logger(Error) << "Database failed";
        return;
    }

    // Setup the db and start using it somewhere after successfully connecting to the server..
    QString dbname = QString::fromStdString("urbansim");
    QString tablename = QString::fromStdString("annual_employment_control_totals");

    QSqlQuery query(db);
    bool sr;
    sr = query.exec("USE "+dbname);

    stringstream ss;

    ss << "CREATE TABLE IF NOT EXISTS ";
    ss << tablename.toStdString();
    ss << " (";
    ss << "year" << " "  << "INT";
    ss << ", ";
    ss << "sector_id" << " "  << "INT";
    ss << ", ";
    ss << "home_based_status" << " "  << "INT";
    ss << ", ";
    ss << "number_of_jobs" << " "  << "INT";
    ss << ")";


    Logger(Debug) << ss.str();
    sr = query.exec(QString::fromStdString(ss.str() ));

    if (!sr) {
        Logger(Error) << query.lastError().text().toStdString();

    }
    stringstream insertstream;
    insertstream << "INSERT INTO " << tablename.toStdString() << "(";
    insertstream << "year";
    insertstream << ", ";
    insertstream << "sector_id";
    insertstream << ", ";
    insertstream << "home_based_status";
    insertstream << ", ";
    insertstream << "number_of_jobs";
    insertstream  << ") " << " VALUES (";

    insertstream << "?";
    insertstream << ", ";
    insertstream << "?";
    insertstream << ", ";
    insertstream << "?" ;
    insertstream << ", ";
    insertstream << "?" ;
    insertstream  << ")";



    for (int y = startYear; y <= endYear; y++) {
            query.prepare(QString::fromStdString(insertstream.str()));
            query.addBindValue(y);
            query.addBindValue(1);
            query.addBindValue(0);
            numberOfJobs = (int) numberOfJobs * this->growthRate;
            query.addBindValue(numberOfJobs);
        if ( !query.exec() )
            Logger(Error) << query.lastError().text().toStdString();
    }

    db.close();

}

std::string AnnualEmploymentControl::getHelpUrl()
{
    return "https://docs.google.com/document/pub?id=1xWU4LpW8VKJrwt_zxiPpycmhT37A-I4W801iBJiFklo";
}
