#include <sqlite3.h>
#include <utility>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <string>
#include <algorithm>
#include <iostream>
#include "database.h"

namespace database {
using namespace std;
constexpr uint32_t IND_ZERO = 0;
constexpr uint32_t IND_ONE = 1;
constexpr uint32_t IND_TWO = 2;
constexpr uint32_t IND_THREE = 3;
constexpr const char* UPDATE_TIMESTAMP = "update_timestamp";
constexpr const char* EXPIRE_TIMESTAMP = "expire_timestamp";

int GetNowTimestamp()
{
    auto time = chrono::system_clock::now();
    int timestamp = chrono::duration_cast<chrono::milliseconds>(time.time_since_epoch()).count();
    return timestamp;
}

bool Database::KeyIsText(string tableName, string key)
{
    if (keysMap.find(tableName) != keysMap.end() && keysMap[tableName].find(key) != keysMap[tableName].end()
        && keysMap[tableName][key] == "TEXT") {
        return true;
    }
    return false;
}
OP_RET Database::OpenDb(string currDbName_, bool enableHis_)
{
    currDbName = currDbName_;
    enableHis = enableHis_;
    int rc1 = sqlite3_open(currDbName.c_str(), &currDb);
    int rc2 = 0;
    if (enableHis) {
        hisDbName = currDbName + "_history";
        rc2 = sqlite3_open(hisDbName.c_str(), &hisDb);
    }
    if (rc1 || rc2) {
        return OP_RET::FAIL;
    } else {
        return OP_RET::SUCCESS;
    }
}
int Database::CreateTableOp(sqlite3 *db, string tableName, vector<tuple<string, string, bool, bool>> params,
                            vector<string> primaryKeys)
{
    char *errMsg;
    string sql = "CREATE TABLE IF NOT EXISTS " + tableName + " (";
    for (int i = 0; i < params.size(); i++) {
        sql += get<IND_ZERO>(params[i]) + " " + get<IND_ONE>(params[i]) +
            (get<IND_TWO>(params[i]) ? " NOT NULL" : " DEFAULT NULL");

        if (i != params.size() - 1) {
            sql += ", ";
        }
    }
    if (!primaryKeys.empty()) {
        sql += ", PRIMARY KEY (";
        for (int i = 0; i < primaryKeys.size(); i++) {
            sql += ((i == 0 ? "" : ", ") + primaryKeys[i]);
        }
        sql += ")";
    }
    sql += ");";
    cout << sql << endl;
    int rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        cout << "create table error: " << errMsg << endl;
    }
    sqlite3_free(errMsg);
    return rc;
}

// void Database::CreateTable(string tableName, vector<tuple<string, string, bool, bool>> createTableParams) {
//     return;
// }
// createTableParams: name, type, not null, primary key
OP_RET Database::CreateTable(string tableName, vector<tuple<string, string, bool, bool>> createTableParams)
{
    for (int i = 0; i < createTableParams.size(); i++) {
        if (get<IND_THREE>(createTableParams[i])) {
            if (primaryKeysMap.find(tableName) != primaryKeysMap.end()) {
                primaryKeysMap[tableName].push_back(get<IND_ZERO>(createTableParams[i]));
            } else {
                primaryKeysMap[tableName] = {get<IND_ZERO>(createTableParams[i])};
            }
        }
        if (keysMap.find(tableName) != keysMap.end()) {
            keysMap[tableName][get<IND_ZERO>(createTableParams[i])] = get<IND_ONE>(createTableParams[i]);
        } else {
            keysMap[tableName] = {{get<IND_ZERO>(createTableParams[i]), get<IND_ONE>(createTableParams[i])}};
        }
    }
    vector<tuple<string, string, bool, bool>> params = createTableParams;
    vector<string> primaryKeys = primaryKeysMap[tableName];
    params.push_back(make_tuple(UPDATE_TIMESTAMP, "INTEGER", true, false));
    keysMap[tableName][UPDATE_TIMESTAMP] = "INTEGER";
    int rc1 = CreateTableOp(currDb, tableName, params, primaryKeys);
    int rc2 = SQLITE_OK;
    if (enableHis) {
        // in his db, UPDATE_TIMESTAMP is primary key
        params[params.size() - 1] = make_tuple(UPDATE_TIMESTAMP, "INTEGER", true, true);
        params.push_back(make_tuple(EXPIRE_TIMESTAMP, "INTEGER", false, false));
        primaryKeys.push_back(UPDATE_TIMESTAMP);
        rc2 = CreateTableOp(hisDb, tableName, params, primaryKeys);
    }
    if (rc1 != SQLITE_OK || rc2 != SQLITE_OK) {
        return OP_RET::FAIL;
    } else {
        return OP_RET::SUCCESS;
    }
}

int QueryDataCallback(void *args, int columnSize, char *columnValue[], char *columnName[])
{
    unordered_map<string, string> data;
    vector<unordered_map<string, string>> *res = (vector<unordered_map<string, string>> *)args;
    for (int i = 0; i < columnSize; i++) {
        if (columnValue[i] == nullptr) {
            data[columnName[i]] = "NULL";
            continue;
        }
        data[columnName[i]] = columnValue[i];
    }
    res->push_back(data);
    return 0;
}

OP_RET Database::QueryCurrData(string tableName, unordered_map<string, pair<COMP_SYMB, string>> data,
                               vector<unordered_map<string, string>> *res)
{
    res->clear();
    vector<pair<string, pair<COMP_SYMB, string>>> queryData(data.begin(), data.end());
    string sql = "SELECT * FROM " + tableName;
    if (queryData.size() > 0) {
        sql += " WHERE ";
    }
    for (int i = 0; i < queryData.size(); i++) {
        if (i != 0) {
            sql += " AND ";
        }
        sql += queryData[i].first + string(queryData[i].second.first);
        if (KeyIsText(tableName, queryData[i].first)) {
            sql += ("'" + queryData[i].second.second + "'");
        } else {
            sql += queryData[i].second.second;
        }
    }
    sql += ";";
    cout << sql << endl;
    char *errMsg;
    int rc = sqlite3_exec(currDb, sql.c_str(), QueryDataCallback, (void *)res, &errMsg);
    if (rc != SQLITE_OK) {
        cout << "query data error: " << errMsg << endl;
        sqlite3_free(errMsg);
        return OP_RET::FAIL;
    } else {
        cout << "query success" << endl;
        return OP_RET::SUCCESS;
    }
}

OP_RET Database::QueryHisData(string tableName, unordered_map<string, pair<COMP_SYMB, string>> data,
                              vector<unordered_map<string, string>> *res, int startTimestamp, int endTimestamp)
{
    res->clear();
    vector<pair<string, pair<COMP_SYMB, string>>> queryData(data.begin(), data.end());
    string sql = "SELECT * FROM " + tableName;
    sql += " WHERE ";
    for (int i = 0; i < queryData.size(); i++) {
        if (i != 0) {
            sql += " AND ";
        }
        sql += queryData[i].first + string(queryData[i].second.first);
        if (KeyIsText(tableName, queryData[i].first)) {
            sql += ("'" + queryData[i].second.second + "'");
        } else {
            sql += queryData[i].second.second;
        }
    }
    if (queryData.size() > 0) {
        sql += " AND ";
    }
    sql += string(UPDATE_TIMESTAMP) + " <= " + to_string(endTimestamp) + " AND (" + string(EXPIRE_TIMESTAMP) +
            " IS NULL OR " + string(EXPIRE_TIMESTAMP) + " >= " + to_string(startTimestamp) + ");";
    cout << sql << endl;
    char *errMsg;
    int rc = sqlite3_exec(hisDb, sql.c_str(), QueryDataCallback, (void *)res, &errMsg);
    if (rc != SQLITE_OK) {
        cout << "query data error: " << errMsg << endl;
        sqlite3_free(errMsg);
        return OP_RET::FAIL;
    } else {
        cout << "query success" << endl;
        return OP_RET::SUCCESS;
    }
}

int Database::UpdateDataOp(sqlite3 *db, string tableName, unordered_map<string, pair<COMP_SYMB, string>> condition,
                           unordered_map<string, string> content)
{
    string sql = "UPDATE " + tableName + " SET ";
    for (auto it = content.begin(); it != content.end(); it++) {
        if (it != content.begin()) {
            sql += ", ";
        }
        if (KeyIsText(tableName, it->first)) {
            sql += (it->first + " = '" + it->second + "'");
        } else {
            sql += it->first + " = " + it->second;
        }
    }
    sql += " WHERE ";
    for (auto it = condition.begin(); it != condition.end(); it++) {
        if (it != condition.begin()) {
            sql += " AND ";
        }
        if (KeyIsText(tableName, it->first)) {
            sql += it->first + string(it->second.first) + "'" + it->second.second + "'";
        } else {
            sql += (it->first + string(it->second.first) + it->second.second);
        }
    }
    sql += ";";
    cout << sql << endl;
    char *errMsg;
    int rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        cout << "update data error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    return rc;
}

OP_RET Database::UpdateData(string tableName, unordered_map<string, pair<COMP_SYMB, string>> condition,
                            unordered_map<string, string> content)
{
    int timestamp = GetNowTimestamp();
    content[UPDATE_TIMESTAMP] = to_string(timestamp);
    int rc = UpdateDataOp(currDb, tableName, condition, content);
    if (rc != SQLITE_OK) {
        return OP_RET::FAIL;
    }
    if (enableHis) {
        unordered_map<string, string> expireContent;
        expireContent[EXPIRE_TIMESTAMP] = to_string(timestamp);
        rc = UpdateDataOp(hisDb, tableName, condition, expireContent);
        if (rc != SQLITE_OK) {
            return OP_RET::FAIL;
        }
        cout << "hello" << endl;
        vector<unordered_map<string, string>> queryRes;
        OP_RET rc2 = QueryCurrData(tableName, condition, &queryRes);
        if (rc2 == OP_RET::FAIL) {
            return OP_RET::FAIL;
        }
        for (int i = 0; i < queryRes.size(); i++) {
            unordered_map<string, string> res = queryRes[i];
            rc = InsertDataOp(hisDb, tableName, res);
            if (rc != SQLITE_OK) {
                return OP_RET::FAIL;
            }
        }
    }
    return OP_RET::SUCCESS;
}

int Database::InsertDataOp(sqlite3 *db, string tableName, unordered_map<string, string> data)
{
    char *errMsg;
    vector<pair<string, string>> insertData(data.begin(), data.end());
    string sql = "INSERT INTO " + tableName + "(";
    for (int i = 0; i < insertData.size(); i++) {
        if (i != 0) {
            sql += ",";
        }
        sql += insertData[i].first;
    }
    sql += ") VALUES(";
    for (int i = 0; i < insertData.size(); i++) {
        if (i != 0) {
            sql += ",";
        }
        if (KeyIsText(tableName, insertData[i].first)) {
            insertData[i].second = ("'" + insertData[i].second + "'");
        } else if (keysMap.find(tableName) == keysMap.end() || keysMap[tableName].find(insertData[i].first) == keysMap[tableName].end()) {
            cout << "insert data error: " << insertData[i].first << " not in " << tableName << endl;
        }
        sql += insertData[i].second;
    }
    sql += ");";
    cout << sql << endl;
    int rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        cout << "insert data error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    return rc;
}

OP_RET Database::InsertData(string tableName, unordered_map<string, string> data)
{
    // if the primary key already exists, update
    std::cout << "start update table " << tableName << std::endl;
    unordered_map<string, pair<COMP_SYMB, string>> queryMap;
    unordered_map<string, string> contentMap;
    for (auto it: data) {
        if (find(primaryKeysMap[tableName].begin(), primaryKeysMap[tableName].end(), it.first)
            != primaryKeysMap[tableName].end()) {
            queryMap[it.first] = make_pair(EQ, it.second);
        } else {
            contentMap[it.first] = it.second;
        }
    }
    vector<unordered_map<string, string>> queryRes;
    OP_RET qrc = Database::QueryCurrData(tableName, queryMap, &queryRes);
    if (qrc == OP_RET::FAIL) {
        return OP_RET::FAIL;
    }
    cout << "query size: " << queryRes.size() << endl;
    for (auto it: queryRes) {
        for (auto it2: it) {
            cout << it2.first << " " << it2.second << endl;
        }
    }
    if (queryRes.size() > 0) {
        return Database::UpdateData(tableName, queryMap, contentMap);
    }
    // else, insert
    cout << "insert" << endl;
    int timestamp = GetNowTimestamp();
    data[UPDATE_TIMESTAMP] = to_string(timestamp);
    int rc = InsertDataOp(currDb, tableName, data);
    if (rc != SQLITE_OK) {
        return OP_RET::FAIL;
    }
    if (enableHis) {
        int rc2 = InsertDataOp(hisDb, tableName, data);
        cout << "his insert success " << rc2 << endl;
        if (rc2 != SQLITE_OK) {
            return OP_RET::FAIL;
        }
    }
    return OP_RET::SUCCESS;
}

int Database::DeleteDataOp(sqlite3 *db, string tableName, unordered_map<string, pair<COMP_SYMB, string>> data)
{
    string sql = "DELETE FROM " + tableName + " WHERE ";
    vector<pair<string, pair<COMP_SYMB, string>>> deleteData(data.begin(), data.end());
    for (int i = 0; i < deleteData.size(); i++) {
        if (i != 0) {
            sql += " AND ";
        }
        sql += deleteData[i].first + deleteData[i].second.first;
        if (KeyIsText(tableName, deleteData[i].first)) {
            sql += "'" + deleteData[i].second.second + "'";
        } else {
            sql += deleteData[i].second.second;
        }
        sql += ";";
    }
    cout << sql << endl;
    char *errMsg;
    int rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        cout << "delete data error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    return rc;
}

// delete a line in curr db, and expire the corresponding line in his db
OP_RET Database::DeleteData(string tableName, unordered_map<string, pair<COMP_SYMB, string>> data)
{
    int rc = DeleteDataOp(currDb, tableName, data);
    if (rc != SQLITE_OK) {
        return OP_RET::FAIL;
    }
    if (enableHis) {
        int timestamp = GetNowTimestamp();
        unordered_map<string, string> content;
        content[EXPIRE_TIMESTAMP] = to_string(timestamp);
        int rc2 = UpdateDataOp(hisDb, tableName, data, content);
        if (rc2 != SQLITE_OK) {
            return OP_RET::FAIL;
        }
    }
    return OP_RET::SUCCESS;
}

OP_RET Database::DropTable(sqlite3 *db, string tableName)
{
    cout << "dropping " << tableName << endl;
    char *errMsg;
    string sql = "DROP TABLE IF EXISTS " + tableName + ";";
    int rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &errMsg);
    if (rc != SQLITE_OK) {
        cout << "drop table error: " << tableName << " " << errMsg << endl;
        sqlite3_free(errMsg);
        return OP_RET::FAIL;
    } else {
        return OP_RET::SUCCESS;
    }
}

OP_RET Database::CloseDb()
{
    cout << "closing db" << endl;
    for (auto it: keysMap) {
        string tableName = it.first;
        OP_RET rc = DropTable(currDb, tableName);
        if (rc == OP_RET::FAIL) {
            return OP_RET::FAIL;
        } else if (enableHis) {
            OP_RET rc2 = DropTable(hisDb, tableName);
            if (rc2 == OP_RET::FAIL) {
                return OP_RET::FAIL;
            }
        }
    }
    int rc1 = sqlite3_close(currDb);
    int rc2 = 0;
    if (enableHis) {
        rc2 = sqlite3_close(hisDb);
    }
    if (rc1 || rc2) {
        return OP_RET::FAIL;
    }
    return OP_RET::SUCCESS;
}
}