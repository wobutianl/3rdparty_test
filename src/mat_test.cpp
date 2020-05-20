/*
 * @Description:
 * @Version: 2.0
 * @Autor: xiaolinz
 * @Date: 2020-04-10 10:18:56
 * @LastEditors: xiaolinz
 * @LastEditTime: 2020-04-10 17:21:49
 */
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <string>

using namespace cv;
using namespace std;

struct test
{
    Mat pos;
};

int main()
{
    test t;
    float *ptr = new float[25];
    ptr[0] = 1;
    ptr[1] = 2;

    Mat mat(5, 5, CV_32F, ptr);
    t.pos = Mat(5, 5, CV_32F, ptr).clone();

    // mat.data = ptr;

    // std::cout << mat.at<float>(0) << endl;

    delete[] ptr;

    cout << t.pos.at<float>(0) << endl;

    struct s_mp
    {
        long id;            // mappoint 编号
        float worldpos[3];  // [3,1] 在世界坐标系中的位置 lon [0] lat[2]
        uint8_t descriptor[32];  // [32] 描述子
        float normalvector[3];   // [1,3] 该关键点时的平均观测方向
        float mindistance;       // 最小观测距离（尺度不变距离）
        float maxdistance;       // 最大观测距离
    };

    // cout << sizeof(s_mp) << endl;
    // cout << sizeof(long);

    vector<vector<int>> a;
    vector<int> bb;
    bb.push_back(1);
    a.push_back(bb);

    vector<vector<int>> b;
    b.insert(b.end(), a.begin(), a.end());

    cout << b[0].size() << endl;
    cout << b[0][0] << endl;

    return 0;
}