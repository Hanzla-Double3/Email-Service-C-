#pragma once
#include <iostream>
#include <memory>
#include <map>
#include <fstream>
#include <vector>

typedef uint32_t ID;
typedef std::map<std::string, uint8_t> Schema;
typedef uint8_t Size;
typedef std::map<std::string, uint64_t> Row;

#define COLUMN_SIZE 32

class Database
{
private:
    std::string dbName;
    uint8_t numberOfColumns;
    ID lastId;
    uint32_t sizePerRow;
    std::shared_ptr<Schema> schema;
    bool updateConfigAfterEveryInsert = 1;

private:
    static std::shared_ptr<Schema> deserializeSchema(char* data, int numberOfCols);
    static char* serializeSchema(std::shared_ptr<Schema> schema, int &len);
    std::shared_ptr<Row> createRow(std::vector<uint64_t> values, std::vector<std::string> names);
    std::vector<std::string> getColumnNames();
public:
    static void createDb(std::initializer_list<std::string> names, std::initializer_list<Size> bytes, const std::string &name);
    static void createDb(std::vector<std::string> names, std::vector<Size> bytes, const std::string &name);
    Database(const std::string &databaseName);
    bool addRow(std::initializer_list<uint64_t> values, std::initializer_list<std::string> names={});
    bool addRow(std::vector<uint64_t> values, std::vector<std::string> names = {});
    bool updateConfig();
    std::shared_ptr<Row> getRow(ID id);
    bool updateRow(ID id, std::initializer_list<uint64_t> values, std::initializer_list<std::string> names={});
    ID getLastId();
};