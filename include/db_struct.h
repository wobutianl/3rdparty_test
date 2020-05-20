/*
 * @Description:
 * @Version: 2.0
 * @Autor: xiaolinz
 * @Date: 2020-04-04 13:10:29
 * @LastEditors: xiaolinz
 * @LastEditTime: 2020-04-04 22:43:17
 */

#pragma once
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

struct MyRecord
{
    int x, y;
    float z;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(x, y, z);
    }
};

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
