/*
 * @Description:
 * @Version: 2.0
 * @Autor: xiaolinz
 * @Date: 2020-04-04 11:52:08
 * @LastEditors: xiaolinz
 * @LastEditTime: 2020-04-09 10:30:31
 */
#pragma once
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/unordered_map.hpp>
#include <iostream>

#include "db_struct.h"

class client
{
   public:
    sqlite3 *db;

    static int callback(void *NotUsed, int argc, char **argv, char **azColName)
    {
        int i;
        for (i = 0; i < argc; i++)
        {
            printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        }
        printf("\n");
        return 0;
    }

    sqlite3 *db_connect(const char *db_name)
    {
        int rc;
        rc = sqlite3_open(db_name, &db);
        return db;
    }

    int db_exec_stmt(const char *sql)
    {
        int rc;
        char *zErrMsg = 0;
        rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            exit(1);
        }
        return 0;
    }

    void blob_exec(const char *sql, const char *blob, const int &blob_size)
    {
        sqlite3_stmt *stmt;
        int rc;

        rc = sqlite3_prepare(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "sql error:%s\n", sqlite3_errmsg(db));
        }
        std::cout << "blob size : " << blob_size << std::endl;
        sqlite3_bind_blob(stmt, 1, blob, blob_size, NULL);

        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    void select_blob2(const char *sql, char *buffer, int &len)
    {
        sqlite3_stmt *stmt;

        sqlite3_prepare(db, sql, -1, &stmt, NULL);

        int result = sqlite3_step(stmt);
        // int id = 0, len = 0;
        // char buffer[100];
        while (result == SQLITE_ROW)
        {
            const void *pReadBolbData = sqlite3_column_blob(stmt, 0);

            len = sqlite3_column_bytes(stmt, 0);

            memcpy(buffer, pReadBolbData, len);

            result = sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }

    void select_blob3(const char *sql, const void *buffer, int &len)
    {
        sqlite3_stmt *stmt;

        sqlite3_prepare(db, sql, -1, &stmt, NULL);

        int result = sqlite3_step(stmt);
        // int id = 0, len = 0;
        // char buffer[100];
        while (result == SQLITE_ROW)
        {
            const void *pReadBolbData = sqlite3_column_blob(stmt, 0);

            len = sqlite3_column_bytes(stmt, 0);

            // memcpy(buffer, pReadBolbData, len);

            result = sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }

    void select_blob(const char *sql)
    {
        sqlite3_stmt *stmt;

        sqlite3_prepare(db, sql, strlen(sql), &stmt, NULL);

        int result = sqlite3_step(stmt);
        int id = 0, len = 0;
        while (result == SQLITE_ROW)
        {
            char cStudentId[42];
            std::cout << "查询到一条记录" << std::endl;

            const void *pReadBolbData = sqlite3_column_blob(stmt, 0);

            len = sqlite3_column_bytes(stmt, 0);

            memcpy(cStudentId, pReadBolbData, len);

            std::cout << "list size : " << sizeof(cStudentId) / sizeof(char)
                      << std::endl;
            std::cout << "len size " << len << std::endl;

            std::string test(cStudentId, len);
            std::cout << "string size : " << test.size() << std::endl;

            std::stringstream ss(test);

            std::cout << "select stringstream size : " << ss.str().size()
                      << std::endl;

            cereal::BinaryInputArchive load(ss);  // Create an input archive

            SomeData myData;

            load(myData);  // Read the data from the archive

            std::cout << "size : " << myData.data.size() << std::endl;
            for (auto &x : myData.data)
            {
                std::cout << "id:" << x.first << std::endl;
                std::cout << x.second.z << std::endl;
                std::cout << x.second.x << std::endl;
                std::cout << x.second.y << std::endl;
            }
            result = sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }
};

// int main(int argc, char **argv)
// {
//     char *db_name = "test.db";

//     db = db_connect(db_name);
//     // rc = db_exec_stmt(sql);
//     db_exec();

//     sqlite3_close(db);
//     return 0;
// }
