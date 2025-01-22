#include "psql.h"
#include "config/config.h"
#include <fstream>

void PSQLTrackingDB::createDatabase()
{
    pqxx::nontransaction nt(*C); // Non-transactional context
    auto res = nt.exec("SELECT 1 FROM pg_database WHERE datname=" + nt.quote(dbName));

    if (res.empty())
    {
        const std::string createDbQuery = "CREATE DATABASE " + C->quote_name(dbName);
        nt.exec(createDbQuery);
    }
}

bool PSQLTrackingDB::connectToPostgresDatabase()
{
    std::string connectionString = "dbname=postgres user=postgres password=" + Config::get("db_pass") +
                                   " hostaddr=" + Config::get("db_ip") + " port=" + Config::get("db_port");
    C = new pqxx::connection(connectionString);
    connectedToPostgresDb = 1;

    if (!C->is_open())
    {
        throw std::runtime_error("Couldn't connect to database");
    }
    return 1;
}

bool PSQLTrackingDB::connectToDatabase()
{
    std::string connectionString = "dbname=" + dbName + " user=postgres password=" + Config::get("db_pass") +
                                   " hostaddr=" + Config::get("db_ip") + " port=" + Config::get("db_port");
    C = new pqxx::connection(connectionString);
    connectedToPostgresDb = 0;

    if (!C->is_open())
    {
        throw std::runtime_error("Couldn't connect to database");
    }
    return 1;
}

bool PSQLTrackingDB::databaseExists()
{
    pqxx::nontransaction N(*C);
    pqxx::result R = N.exec("SELECT 1 FROM pg_database WHERE datname = " + N.quote(dbName) + ";");
    return !R.empty();
}

void PSQLTrackingDB::disconnectFromDatabase()
{
    C->disconnect();
}

bool PSQLTrackingDB::tableExists(const std::string &tableName)
{
    pqxx::nontransaction N(*C);
    pqxx::result R = N.exec("SELECT 1 FROM information_schema.tables WHERE table_schema = 'public' AND table_name = " + N.quote(tableName) + ";");
    return !R.empty();
}

void PSQLTrackingDB::createTable(const char *query)
{
    pqxx::work W(*C);
    W.exec(query);
    W.commit();
}

bool PSQLTrackingDB::indexExists(const std::string &indexName)
{
    pqxx::nontransaction N(*C);
    pqxx::result R = N.exec(
        "SELECT 1 FROM pg_indexes WHERE indexname = " + N.quote(indexName) + ";");
    return !R.empty();
}

PSQLTrackingDB *PSQLTrackingDB::getInstance()
{
    if (trackingDb && trackingDb->C && !trackingDb->connectedToPostgresDb){
        return trackingDb;
    }
    trackingDb = new PSQLTrackingDB;
    trackingDb->connectToPostgresDatabase();
    if (!trackingDb->databaseExists())
    {
        trackingDb->createDatabase();
    }
    trackingDb->disconnectFromDatabase();
    trackingDb->connectToDatabase();

    for (size_t i = 0; i < trackingDb->tableCreationQueries.size(); i++)
    {
        if (i < trackingDb->tableNames.size())
        {
            // For tables
            if (!trackingDb->tableExists(trackingDb->tableNames[i]))
            {
                trackingDb->createTable(trackingDb->tableCreationQueries[i]);
            }
        }
        else
        {
            // For indexes (e.g., the 3rd query)
            const std::string indexName = "idx_email_read_data_email_id";
            if (!trackingDb->indexExists(indexName))
            {
                pqxx::work W(*trackingDb->C);
                W.exec(trackingDb->tableCreationQueries[i]);
                W.commit();
            }
        }
    }

    return trackingDb;
}

int PSQLTrackingDB::addMessage()
{
    try
    {
        pqxx::work txn(*C);
        pqxx::result res = txn.exec("INSERT INTO email DEFAULT VALUES RETURNING id;");
        txn.commit();
        return res[0][0].as<int>();
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Add email failed: " + std::string(e.what()));
    }
}

void PSQLTrackingDB::readMessage(int email_id)
{
    try
    {
        pqxx::work txn(*C);
        
        // Update read count in email table
        txn.exec_params(
            "UPDATE email SET read_times = read_times + 1 WHERE id = $1",
            email_id);

        // Add read entry
        txn.exec_params(
            "INSERT INTO email_read_data (email_id) VALUES ($1)",
            email_id);

        txn.commit();
    }
    catch (const pqxx::sql_error& e) {
        if (e.sqlstate() == "23503") {
            // Rollback transaction automatically
            return;  // Silent failure
        }
        throw;
    }
}

std::vector<ReadData> PSQLTrackingDB::timesReadMessage(int email_id)
{
    std::vector<ReadData> results;
    try
    {
        pqxx::read_transaction txn(*C);
        auto res = txn.exec_params(
            "SELECT read_on, count FROM email_read_data WHERE email_id = $1",
            email_id);

        for (const auto &row : res)
        {
            results.push_back({row["read_on"].as<std::string>(),
                               row["count"].as<int>()});
        }
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Get read data failed: " + std::string(e.what()));
    }
    return results;
}

void PSQLTrackingDB::deleteInstance()
{
    delete trackingDb;
}

PSQLTrackingDB::~PSQLTrackingDB()
{
    delete C;
}

PSQLTrackingDB *PSQLTrackingDB::trackingDb = nullptr;