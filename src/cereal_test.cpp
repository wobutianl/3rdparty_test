/*
 * @Description:
 * @Version: 2.0
 * @Autor: xiaolinz
 * @Date: 2020-04-02 15:12:13
 * @LastEditors: xiaolinz
 * @LastEditTime: 2020-04-14 10:00:01
 */
#include <sqlite3.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/unordered_map.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "db_struct.h"
#include "smartdb/database.hpp"
#include "sqlite_binary_test.h"

std::string sql;
smartdb::database db;
bool ok;

// struct MyRecord
// {
//     uint8_t x, y;
//     float z;

//     template <class Archive>
//     void serialize(Archive& ar)
//     {
//         ar(x, y, z);
//     }
// };

struct SomeData
{
    int32_t id;
    std::unordered_map<uint32_t, MyRecord> data;

    template <class Archive>
    void save(Archive& ar) const
    {
        ar(id, data);
    }

    template <class Archive>
    void load(Archive& ar)
    {
        static int32_t idGen = 0;
        id = idGen++;
        ar(id, data);
    }
};

using namespace std;

static int callback(void* NotUsed, int argc, char** argv, char** azColName)
{
    int i;
    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

void open_db()
{
    sqlite3* db;
    char* zErrMsg = 0;
    int rc;

    rc = sqlite3_open("test.db", &db);

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }
    else
    {
        fprintf(stderr, "Opened database successfully\n");
    }

    //******************** */
    std::string sql =
        "CREATE TABLE if not exists temps (id INTEGER PRIMARY KEY "
        "AUTOINCREMENT, value blob);";

    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Table created successfully\n");
    }

    //******************** */
    sql = "select value from temps;";
    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        fprintf(stdout, "Records created successfully\n");
    }

    sqlite3_close(db);
}

void save_db(std::stringstream& sd)
{
    ok = db.open_file_db("temp.db");
    sql =
        "CREATE TABLE if not exists temp(id INTEGER PRIMARY KEY "
        "AUTOINCREMENT, value blob );";

    ok = db.execute(sql);
    if (ok)
    {
        std::cout << "make table OK" << std::endl;
    }
    else
    {
        std::cout << "error in make keyframe" << std::endl;
    }

    std::string sql =
        "INSERT INTO temp(value) "
        "VALUES(?)";

    std::cout << "111111";

    // 预处理sql.
    ok = db.prepare(sql);
    // 开始事务.
    ok = db.begin();
    bool ret = true;

    smartdb::db_blob mp_value;

    mp_value.buf = sd.str().c_str();
    mp_value.size = sd.str().size();

    ret = db.add_bind_value(mp_value);

    if (ret)
    {
        ok = db.commit();
    }
    else
    {
        ok = db.rollback();
    }
}

void select_data(SomeData& sd)
{
    std::cout << "--------------------" << std::endl;
    ok = db.open_file_db(
        "/home/xiaolinz/uisee_blog/test/test_cereal/build/temp.db");
    if (ok)
    {
        std::cout << "43243423423" << std::endl;
    }
    else
    {
        std::cout << "--------------" << std::endl;
    }

    sql = "select value from temp;";

    ok = db.execute(sql.c_str());

    bool ret = true;

    std::cout << db.record_count() << std::endl;

    while (!db.is_end())
    {
        try
        {
            std::string value = db.get<std::string>(0);

            std::cout << "4444444   " << value.size() << std::endl;

            std::stringstream ss(value);

            std::cout << "555555 " << std::endl;

            cereal::BinaryInputArchive iarchive(ss);
            std::cout << "6666   ss size : " << std::endl;

            SomeData aaaa;
            iarchive(aaaa);

            std::cout << "90909090     size:   " << aaaa.data.size()
                      << std::endl;

            std::cout << "output value : " << aaaa.data[0].x << "----------"
                      << std::endl;
        }
        catch (std::exception& e)
        {
            std::cout << "Exception: " << e.what() << std::endl;
            return;
        }
        db.move_next();
    }
}

//************************************** */

int main()
{
    client clt;
    sqlite3* db = clt.db_connect("temp.db");
    //*************** */
    sql =
        "CREATE TABLE if not exists temps (id INTEGER PRIMARY KEY "
        "AUTOINCREMENT, value blob);";

    clt.db_exec_stmt((char*)sql.c_str());

    //************* */
    std::stringstream os;

    // std::ofstream os("out.cereal", std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);

    SomeData myData;
    myData.id = 10;

    MyRecord mr;
    mr.x = 111;
    mr.y = 222;
    mr.z = 10;

    MyRecord mr2;
    mr2.x = 2121;
    mr2.y = 4444;
    mr2.z = 100;

    myData.data.insert(std::make_pair(1, mr));
    myData.data.insert(std::make_pair(2, mr2));

    oarchive(myData);

    sql =
        "INSERT INTO temps(value) "
        "VALUES(?)";

    std::string insert_value = os.str();

    clt.blob_exec(sql.c_str(), insert_value.c_str(), insert_value.size());

    //************** */
    char buffer2[1024] = "0";
    std::string value;
    sql = "select value from temps;";
    clt.select_blob(sql.c_str());

    std::cout << "value size : " << value.size() << std::endl;

    // open_db();

    // save_db(os);

    // select_data(myData);

    {
        // std::stringstream ss(value);

        // cereal::BinaryInputArchive iarchive(ss);  // Create an input archive

        // SomeData myData;

        // iarchive(myData);  // Read the data from the archive

        // std::cout << myData.id << "----" << myData.data[0].z << std::endl;
    }

    std::stringstream ss;
    // any stream can be used

    {
        // Create an output archive
        cereal::BinaryOutputArchive oarchive(ss);

        MyRecord m1, m2, m3;
        m1.x = 1;
        m2.y = 1;
        m3.z = 2222;

        oarchive(m1, m2, m3);  // Write the data to the archive
    }  // archive goes out of scope, ensuring all contents are flushed

    {
        cereal::BinaryInputArchive iarchive(ss);  // Create an input archive

        MyRecord m1, m2, m3;

        iarchive(m1, m2, m3);  // Read the data from the archive

        std::cout << int(m1.x) << "----" << int(m2.y) << "------" << m3.z
                  << std::endl;
        // std::cout << tv.value[0].x << "----" << tv.value[1].y;
    }

    return 0;
}