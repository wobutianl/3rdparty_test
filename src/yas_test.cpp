/*
 * @Description:
 * @Version: 2.0
 * @Autor: xiaolinz
 * @Date: 2020-04-08 22:21:56
 * @LastEditors: xiaolinz
 * @LastEditTime: 2020-04-14 10:00:20
 */
#include <iostream>
#include <opencv2/core.hpp>
#include <string>
#include <yas/serialize.hpp>
#include <yas/std_types.hpp>

#include "sqlite_binary_test.h"

template <typename _Tp>
std::vector<_Tp> convertMat2Vector(const cv::Mat &mat)
{
    //通道数不变，按行转为一行
    return (std::vector<_Tp>)(mat.reshape(1, 1));
}

template <typename _Tp>
cv::Mat convertVector2Mat(std::vector<_Tp> &v, int channels, int rows)
{
    //将vector变成单列的mat
    cv::Mat mat = cv::Mat(v);
    // PS：必须clone()一份，否则返回出错
    cv::Mat dest = mat.reshape(channels, rows).clone();
    return dest;
}

struct type
{
    type() : i(33), d(.34), s("35"), v({36, 37, 38}) {}

    std::uint32_t i;
    double d;
    std::string s;
    std::vector<int> v;
};

// one free-function serializer/deserializer
template <typename Ar>
void serialize(Ar &ar, type &t)
{
    if (Ar::is_writable())
    {  // constexpr
        ar &YAS_OBJECT_NVP("type", ("i", t.i), ("d", t.d), ("s", t.s),
                           ("v", t.v));
    }
    else
    {
        ar &YAS_OBJECT_NVP("type", ("i", t.i), ("d", t.d), ("s", t.s),
                           ("v", t.v));
    }
}

using namespace std;
int main()
{
    std::string db = "yas.db";
    client clt;
    clt.db_connect(db.c_str());

    std::string sql;

    // sql = "create table if not exists temp (value blob);";
    // clt.db_exec_stmt(sql.c_str());

    type t1, t2;
    t2.i = 0;
    t2.d = 0;
    t2.s.clear();
    t2.v.clear();

    constexpr std::size_t flags = yas::mem | yas::binary;

    yas::shared_buffer buf = yas::save<flags>(t1);

    // sql = "insert into temp (value) values(?);";
    // clt.blob_exec(sql.c_str(), buf.data.get(), buf.size);

    sql = "select value from temp limit 1";

    shared_ptr<char> buffer(new char(100));
    int len;
    clt.select_blob2(sql.c_str(), buffer.get(), len);

    yas::shared_buffer temp;
    temp.data = buffer;
    temp.size = len;

    yas::load<flags>(temp, t2);

    cout << t2.i << endl;
    cout << t2.d << endl;
    cout << t2.s << endl;
    for (auto &x : t2.v) cout << x << endl;

    // TODO: stackoverflow.com/questions/17333
    assert(t1.i == t2.i && t1.d == t2.d && t1.s == t2.s);

    // int a = 3, aa{};
    // short b = 4, bb{};
    // float c = 3.14, cc{};
    // cv::Mat gps = (cv::Mat_<double>(1, 3) << 0, -1, 0);

    // cv::Mat ggps;

    // std::vector<double> fgps, vgps{};
    // // fgps = convertMat2Vector<double>(gps);

    // fgps.push_back(1);
    // fgps.push_back(2);

    // std::string db = "yas.db";
    // client clt;
    // clt.db_connect(db.c_str());

    // type t1, t2;
    // t2.i = 0;
    // t2.d = 0;
    // t2.s.clear();
    // t2.v.clear();

    // constexpr std::size_t flags = yas::mem        // IO type
    //                               | yas::binary;  // IO format

    // yas::shared_buffer buf = yas::save<flags>(t1);

    // // buf = {"a":3,"b":4,"c":3.14}

    // // std::string sql = "create table if not exists temp (value blob);";
    // // clt.db_exec_stmt(sql.c_str());

    // // sql = "insert into temp (value) values(?);";
    // // clt.blob_exec(sql.c_str(), buf.data.get(), buf.size);

    // string sql = "select value from temp limit 1";

    // cout << "-----------" << endl;

    // shared_ptr<char> buffer(new char(100));
    // int len;
    // clt.select_blob2(sql.c_str(), buffer.get(), len);

    // yas::shared_buffer temp;
    // temp.data = buffer;
    // temp.size = len;

    // cout << "-----------" << endl;

    // yas::load<flags>(temp, t2);
    // cout << t2.s.size() << endl;
    // cout << t2.v[0] << endl;

    // a == aa &&b == bb &&c == cc;

    // cout << aa << " --- " << bb << "---" << cc << "-----" << vgps.size()
    //      << endl;

    // ggps = convertVector2Mat<double>(vgps, 3, 1);
    // cout << ggps.size() << endl;
}