/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
 * witty-ub is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

namespace database {
using namespace std;
enum OP_RET {
    SUCCESS = 0,
    FAIL = 1,
    NOT_FOUND = 2,
};
using COMP_SYMB = const char*;
constexpr COMP_SYMB EQ = "=";
constexpr COMP_SYMB NEQ = "!=";
constexpr COMP_SYMB LT = "<";
constexpr COMP_SYMB LE = "<=";
constexpr COMP_SYMB GT = ">";
constexpr COMP_SYMB GE = ">=";
constexpr COMP_SYMB IN = " IS NOT ";


class Database {
public:
    OP_RET OpenDb(string currDbName_, bool enableHis_);
    // name, type, null, primary_key
    OP_RET CreateTable(string tableName, vector<tuple<string, string, bool, bool>> createTableParams);
    OP_RET InsertData(string tableName, unordered_map<string, string> data);
    OP_RET UpdateData(string tableName, unordered_map<string, pair<COMP_SYMB, string>> condition,
                      unordered_map<string, string> content);
    OP_RET DeleteData(string tableName, unordered_map<string, pair<COMP_SYMB, string>> data);
    OP_RET QueryCurrData(string tableName, unordered_map<string, pair<COMP_SYMB, string>> data,
                         vector<unordered_map<string, string>> *res);
    OP_RET QueryHisData(string tableName, unordered_map<string, pair<COMP_SYMB, string>> data,
                        vector<unordered_map<string, string>> *res, int startTimestamp, int endTimestamp);
    OP_RET CloseDb();
private:
    sqlite3 *currDb;
    sqlite3 *hisDb;
    string currDbName;
    string hisDbName;
    bool enableHis;
    unordered_map<string, vector<string>> primaryKeysMap;
    unordered_map<string, unordered_map<string, string>> keysMap;
    bool KeyIsText(string tableName, string key);
    int CreateTableOp(sqlite3 *db, string tableName, vector<tuple<string, string, bool, bool>> params,
                      vector<string> primaryKeys);
    int InsertDataOp(sqlite3 *db, string tableName, unordered_map<string, string> data);
    int UpdateDataOp(sqlite3 *db, string tableName, unordered_map<string, pair<COMP_SYMB, string>> condition,
                     unordered_map<string, string> content);
    int DeleteDataOp(sqlite3 *db, string tableName, unordered_map<string, pair<COMP_SYMB, string>> data);
    OP_RET DropTable(sqlite3 *db, string tableName); 
};
int GetNowTimestamp();
}


#endif // DATABASE_H