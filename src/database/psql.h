#pragma once 
#include <pqxx/pqxx>

struct ReadData {
    std::string read_time;
    int count;
};

class PSQLTrackingDB{
    pqxx::connection* C = nullptr;
    static PSQLTrackingDB* trackingDb;

    std::string dbName = "EmailTracking";
    std::array<const char*, 2> tableNames = {"email", "email_read_data"};
    std::array<const char*, 2> tableCreationQueries = {
        "CREATE TABLE email (id SERIAL PRIMARY KEY, sentOn TIMESTAMP DEFAULT CURRENT_TIMESTAMP, read_times INT DEFAULT 0);",
        "CREATE TABLE email_read_data ( id SERIAL PRIMARY KEY, email_id INT REFERENCES email(id), readOn TIMESTAMP DEFAULT CURRENT_TIMESTAMP, count INT);"
    };
    bool connectedToPostgresDb;

    void createDatabase();
    bool connectToPostgresDatabase();
    bool connectToDatabase();
    bool databaseExists();
    void disconnectFromDatabase();
    bool tableExists(const std::string &tableName);
    void createTable(const char *query);
    bool indexExists(const std::string& indexName);


public:
    static PSQLTrackingDB* getInstance();
    int addMessage();
    void readMessage(int email_id);
    std::vector<ReadData> timesReadMessage(int email_id);
    static void deleteInstance();
    ~PSQLTrackingDB();

};

