#include "annualhouseholdcontroltotals.h"

#include <QtSql>
#include <QUuid>

#include <sstream>

DM_DECLARE_NODE_NAME(AnnualHouseholdControlTotals, PowerVIBe)

AnnualHouseholdControlTotals::AnnualHouseholdControlTotals()
{
    this->startYear = 2011;
    this->endYear = 2020;
    this->growthRate = 1.01;

    this->addParameter("StartYear", DM::INT, &startYear);
    this->addParameter("EndYear", DM::INT, &endYear);
    this->addParameter("GrowthRate", DM::DOUBLE, &this->growthRate);

    city = DM::View("CITY", DM::FACE, DM::MODIFY);
    city.getAttribute("HH01");
    city.getAttribute("HH02");
    city.getAttribute("HH03");
    city.getAttribute("HH04");
    city.getAttribute("HH05");
    std::vector<DM::View> data;
    data.push_back(city);
    this->addData("City", data);

}

void AnnualHouseholdControlTotals::run()
{
    DM::System * sys = this->getData("City");
    std::vector<std::string> uuids = sys->getUUIDsOfComponentsInView(city);
    DM::Component * city = sys->getComponent(uuids[0]);

    int households[5];

    households[0] = (int)city->getAttribute("HH01")->getDouble();
    households[1] = (int)city->getAttribute("HH02")->getDouble();
    households[2] = (int)city->getAttribute("HH03")->getDouble();
    households[3] = (int)city->getAttribute("HH04")->getDouble();
    households[4] = (int)city->getAttribute("HH05")->getDouble();

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
    QString tablename = QString::fromStdString("annual_household_control_totals");

    QSqlQuery query(db);
    bool sr;
    sr = query.exec("USE "+dbname);

    stringstream ss;

    ss << "CREATE TABLE IF NOT EXISTS ";
    ss << tablename.toStdString();
    ss << " (";
    ss << "year" << " "  << "INT";
    ss << ", ";
    ss << "persons" << " "  << "INT";
    ss << ", ";
    ss << "total_number_of_households" << " "  << "INT";
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
    insertstream << "persons";
    insertstream << ", ";
    insertstream << "total_number_of_households";
    insertstream  << ") " << " VALUES (";

    insertstream << "?";
    insertstream << ", ";
    insertstream << "?";
    insertstream << ", ";
    insertstream << "?" ;
    insertstream  << ")";

    for (int y = startYear; y <= endYear; y++) {
        for (int i = 0; i < 5; i++) {
            query.prepare(QString::fromStdString(insertstream.str()));
            query.addBindValue(y);
            query.addBindValue(i+1);
            households[i] = (int) households[i] * this->growthRate;
            query.addBindValue(households[i]);
            if ( !query.exec() )
                Logger(Error) << query.lastError().text().toStdString();
        }
    }
    db.close();
}
