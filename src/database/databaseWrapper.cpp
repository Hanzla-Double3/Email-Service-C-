#pragma once
#include "database.h"
#include <filesystem>
#include <chrono>
#include <ctime>

class TrackingDatabase
{
private:
    Database *db1 = nullptr; // {"date_created", "count"}, {8, 4}
    std::string dbName1;
    Database *db2 = nullptr; // {"fk_email_id", "date_read", "count_number"}, {8, 8, 4}
    std::string dbName2;
    std::string dbDirectory;

    static TrackingDatabase* trackingdb;

    uint64_t getCurrentEpoch()
    {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        return epoch;
    }

    void init()
    {
        std::filesystem::create_directories(dbDirectory); // Ensure the directory exists

        if (db1 == nullptr)
        {
            std::string db1Path = dbDirectory + "/" + dbName1;
            if (!std::filesystem::exists(db1Path))
            {
                Database::createDb({"date_created", "count"}, {8, 4}, db1Path);
            }
            db1 = new Database(db1Path);
        }

        if (db2 == nullptr)
        {
            std::string db2Path = dbDirectory + "/" + dbName2;
            if (!std::filesystem::exists(db2Path))
            {
                Database::createDb({"fk_email_id", "date_read", "count_number"}, {4, 8, 4}, db2Path);
            }
            db2 = new Database(db2Path);
        }
    }

public:
    TrackingDatabase(const std::string& directory = "databases")
        : dbName1("TrackingDatabase1"), dbName2("TrackingDatabase2"), dbDirectory(directory), db1(nullptr), db2(nullptr) {}

    uint32_t addMessage()
    {
        uint64_t epoch = getCurrentEpoch();
        db1->addRow({epoch, 0}, {"date_created", "count"});
        return db1->getLastId();
    }
    
    void readMessage(uint32_t id)
    {
        try {
            auto epoch = getCurrentEpoch();
            auto row = db1->getRow(id);
            uint32_t count = (*row.get())["count"];
            db2->addRow({id, epoch, ++count}, {"fk_email_id", "date_read", "count_number"});
            db1->updateRow(id, {count}, {"count"});
        }
        catch(const std::runtime_error& e) {
            std::cerr << "Caught a runtime_error: " << e.what() << std::endl;
        }
    }
    
    uint64_t timesReadMessage(uint32_t id)
    {
        try {
            auto info = db1->getRow(id);
            return (uint32_t)(*info.get())["count"];
        }
        catch(const std::runtime_error& e) {
            std::cerr << "Caught a runtime_error: " << e.what() << std::endl;
        }
        return -1;
    }
    
    static TrackingDatabase* getInstance(const std::string& directory = "databases")
    {
        if (trackingdb == nullptr) {
            TrackingDatabase* db = new TrackingDatabase(directory);
            db->init();
            trackingdb = db;
        }
        return trackingdb;
    }
    
    static void deleteInstance()
    {
        delete trackingdb->db1;
        delete trackingdb->db2;
        delete trackingdb;
        trackingdb = nullptr;
    }
};

TrackingDatabase* TrackingDatabase::trackingdb = nullptr;
