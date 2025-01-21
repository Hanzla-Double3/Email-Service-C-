#include "database.h"
#include <string.h>

void Database::createDb(std::initializer_list<std::string> names, std::initializer_list<Size> bytes, const std::string &name)
{
    std::vector<std::string> namesVec;
    for(const auto& name: names){
        namesVec.push_back(name);
    }

    std::vector<Size> bytesVec;
    for(const auto& byte: bytes){
        bytesVec.push_back(byte);
    }

    Database::createDb(namesVec, bytesVec, name);
}

std::shared_ptr<Schema> Database::deserializeSchema(char *data, int numberOfCols)
{
    auto schema = std::make_shared<Schema>();
    for (int i = 0; i < numberOfCols; i++)
    {
        int offset = i * (COLUMN_SIZE + sizeof(Size));
        (*schema.get())[(data + offset)] = (Size)data[offset + COLUMN_SIZE];
    }
    return schema;
}

Database::Database(const std::string &databaseName)
{
    this->dbName = databaseName;
    this->sizePerRow = sizeof(ID);
    std::ifstream file(databaseName + ".dbcon", std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open the database file.");
    }

    file.read((char *)&this->lastId, sizeof(this->lastId));
    file.read((char *)&this->numberOfColumns, sizeof(this->numberOfColumns));
    int sizePerEntry = COLUMN_SIZE + sizeof(Size);
    int sizeOfSchema = sizePerEntry * this->numberOfColumns;
    char *serializedSchema = new char[sizeOfSchema];
    file.read(serializedSchema, sizeOfSchema);
    this->schema = Database::deserializeSchema(serializedSchema, this->numberOfColumns);
    for (const auto &[key, value] : *(this->schema.get()))
    {
        this->sizePerRow += value;
    }
    file.close();
}

char *Database::serializeSchema(std::shared_ptr<Schema> schema, int &len)
{
    uint8_t number_of_columns = schema.get()->size();
    //         col name      col size                    number of col
    len = number_of_columns * COLUMN_SIZE + number_of_columns * (sizeof(Size)) + sizeof(Size);
    char *data = new char[len];
    memset(data, 0, len);
    data[0] = number_of_columns;
    int idx = 0;
    for (const auto &[key, value] : *schema.get())
    {
        //           numberOfCol          prev keys and values
        int offset = sizeof(uint8_t) + (idx * (COLUMN_SIZE + sizeof(Size)));
        memcpy(
            data + offset,
            key.c_str(),
            key.size());
        offset += COLUMN_SIZE;
        data[offset + COLUMN_SIZE] = '\0';
        memcpy(
            data + offset,
            (char *)&value,
            sizeof(Size));
        idx++;
    }
    return data;
}

std::shared_ptr<Row> Database::createRow(std::vector<uint64_t> values, std::vector<std::string> names)
{
    std::shared_ptr<Row> row = std::make_shared<Row>();
    auto it = values.begin();
    for (const auto &item : names)
    {
        (*row.get())[item] = *it++;
    }
    return row;
}

std::vector<std::string> Database::getColumnNames()
{
    std::vector<std::string> columnNames;
    for (const auto &[key, value] : *schema.get())
    {
        columnNames.push_back(key);
    }
    return columnNames;
}

bool Database::addRow(std::initializer_list<uint64_t> values, std::initializer_list<std::string> names)
{
    std::vector<std::uint64_t> valuesVec;
    for(const auto& value: values){
        valuesVec.push_back(value);
    }
    
    if (names.size() == 0){
        return this->addRow(valuesVec);
    }
    std::vector<std::string> namesVec;
    for(const auto& name: names){
        namesVec.push_back(name);
    }
    return this->addRow(valuesVec, namesVec);
}

bool Database::updateConfig()
{
    std::ofstream file(dbName + ".dbcon", std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Couldn't update config file");
    }

    file.write(reinterpret_cast<char *>(&lastId), sizeof(lastId));

    int size = 0;
    char *serializedSchema = Database::serializeSchema(schema, size);
    file.write(serializedSchema, size);
    delete[] serializedSchema;
    file.close();
    return 1;
}

std::shared_ptr<Row> Database::getRow(ID id)
{
    if (id > lastId)
    {
        throw std::runtime_error("Id is greater than last id");
    }
    std::ifstream file(dbName);
    file.seekg((id - 1) * sizePerRow);

    auto row = std::make_shared<Row>();

    char *Id = new char[sizeof(ID)];
    file.read(Id, sizeof(ID));
    (*row.get())["id"] = *(ID *)Id;
    delete[] Id;

    for (const auto &[key, value] : *(schema.get()))
    {
        char *val = new char[value];
        file.read(val, value);
        (*row.get())[key] = *(uint64_t *)val;
        delete[] val;
    }
    return row;
}

bool Database::updateRow(ID id, std::initializer_list<uint64_t> values, std::initializer_list<std::string> names)
{
    // if a key is not present in newRow, it should stay same
    if (id > lastId)
    {
        throw std::runtime_error("Id is greater than last id");
    }

    std::vector<std::string> columnNames;
    std::vector<uint64_t> valuesVec;
    if (names.size() == 0)
    {
        columnNames = getColumnNames();
    }
    else
    {
        columnNames = names;
    }
    valuesVec = values;
    auto row = this->createRow(values, names);

    std::fstream file(dbName, std::ios::in | std::ios::out | std::ios::binary);
    std::streampos pos = (id - 1) * sizePerRow + sizeof(id);
    file.seekg(pos);

    for (const auto &[key, value] : *(schema.get()))
    {
        if (row->find(key) == row->end())
        {
            pos += value;
            file.seekg(pos);
        }
        else
        {
            file.write((char *)&(*(row.get()))[key], value);
        }
    }
    return 1;
}

ID Database::getLastId()
{
    return this->lastId;
}

void Database::createDb(std::vector<std::string> names, std::vector<Size> bytes, const std::string &name)
{
    std::ofstream configFile(name + ".dbcon", std::ios::binary | std::ios::trunc);
    if (!configFile.is_open())
    {
        throw std::runtime_error("Couldn't create config file");
    }

    ID lastId = 0;
    configFile.write(reinterpret_cast<char *>(&lastId), sizeof(lastId));

    std::shared_ptr<Schema> schema = std::make_shared<Schema>();
    auto it = bytes.begin();
    for (const auto &item : names)
    {
        if (item.size() > COLUMN_SIZE - 1)
        {
            throw std::runtime_error("Column name should be less that 31 characher");
        }
        (*schema.get())[item] = *it++;
    }

    int size = 0;
    char *serializedSchema = Database::serializeSchema(schema, size);
    configFile.write(serializedSchema, size);
    delete[] serializedSchema;
    configFile.close();
}

bool Database::addRow(std::vector<uint64_t> values, std::vector<std::string> names)
{
    int keySize = this->schema->size();
    if (values.size() != keySize)
    {
        throw std::runtime_error("Values aren't equal to key size");
    }

    std::vector<std::string> columnNames;
    std::vector<uint64_t> valuesVec;
    if (names.size() == 0)
    {
        columnNames = getColumnNames();
    }
    else
    {
        columnNames = names;
    }
    valuesVec = values;
    auto row = this->createRow(values, names);

    std::ofstream file(dbName, std::ios::binary | std::ios::app);
    if (!file.is_open())
    {
        throw std::runtime_error("Couldn't open database file");
    }
    lastId++;
    file.write((char *)&lastId, sizeof(lastId));
    char c = '\0';
    for (const auto &[key, value] : *(schema.get()))
    {
        if (row->find(key) == row->end())
        {
            for (int i = 0; i < value; i++)
            {
                file.write(&c, 1);
            }
        }
        else
        {
            file.write((char *)&(*(row.get()))[key], value);
        }
    }
    file.close();
    if (this->updateConfigAfterEveryInsert){
        this->updateConfig();
    }
    return 1;
}
