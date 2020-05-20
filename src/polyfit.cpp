

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <vector>
#include <iterator>
#include <shapefil.h>

#include <regex>

// using namespace cv;
using namespace std;

// http://download.osgeo.org/shapelib/
 

 
struct Point
{
	Point()
	{
	    x = 0.0;
	    y = 0.0;
	    z = 0.0;
	}

	Point(double _x, double _y, double _z)
	{
	    x = _x;
	    y = _y;
	    z = _z;
	}

	double x;
	double y;
	double z;
};

cv::Mat polyfit(vector<cv::Point>& in_point, int n);
void writeShapeFile(vector<Point>& line_shp);

int main()
{
    string origin_data = " [ , 13.896496 ], [ , 13.895054 ], [ , 13.894778 ], [ , 13.893112 ], [ , 13.890326 ], [ , 13.887539 ], [ , 13.884753 ], [ , 13.883 ], [ , 13.884091 ], [ , 13.886644 ], [ , 13.891797 ], [ , 13.899811 ], [ 1018523.446782, 3461328.86303499992937, 13.911982 ], [ 1018523.201326, 3461330.844177, 13.927519 ], [ 1018522.94032, 3461332.822922, 13.942697 ], [ 1018522.679312, 3461334.80172599991783, 13.955498 ], [ 1018522.432229, 3461336.782004, 13.959883 ], [ 1018522.18769699998666, 3461338.76310099987313, 13.957081 ], [ 1018521.9480029999977, 3461340.744314, 13.948307 ], [ 1018521.70152799994685, 3461342.725509, 13.939602 ], [ 1018521.45075, 3461344.70541599998251, 13.938329 ], [ 1018521.20694399997592, 3461346.685963, 13.944325 ], [ 1018520.96427899994887, 3461348.667177, 13.954518 ], [ 1018520.71018399996683, 3461350.646811, 13.967472 ], [ 1018520.455665, 3461352.626627, 13.978167 ], [ 1018520.204808, 3461354.606815, 13.984382 ], [ 1018519.95316699997056, 3461356.603, 13.989 ], [ 1018519.70807399996556, 3461358.56724399980158, 13.99 ], [ 1018519.46255, 3461360.54845499992371, 13.986871 ], [ 1018519.21734299999662, 3461362.528717, 13.978603 ], [ 1018518.97424699994735, 3461364.509881, 13.967506 ], [ 1018518.7318199999863, 3461366.491094, 13.957611 ], [ 1018518.489393, 3461368.47230699984357, 13.953416 ], [ 1018518.23837799998, 3461370.452508, 13.956008 ], [ 1018517.952224, 3461372.42819399992004, 13.962533 ], [ 1018517.713798, 3461374.409975, 13.962944 ], [ 1018517.711021165829152, 3461374.444575409404933, 13.962848914481574 ] ]";
    
    
    // regex word_regex("(-?d+.d+)");
    // auto words_begin = std::sregex_iterator(origin_data.begin(), origin_data.end(), word_regex);
    // auto words_end = std::sregex_iterator();
 
    // std::cout << "Found "
    //           << std::distance(words_begin, words_end)
    //           << " words\n";

    // for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
    //     std::smatch match = *i;
    //     std::string match_str = match.str();
    //     cout<<match_str<<endl;
    // }

    
    //数据输入
    cv::Point in[19] = { cv::Point(1018528.145521628903225, 3461291.902215837966651),cv::Point( 1018527.96805699996185, 3461293.222664),cv::Point(1018527.70256699994206, 3461295.200848),cv::Point(1018527.44723599997815, 3461297.18065099976957),cv::Point(1018527.197628, 3461299.161098)
		,cv::Point(1018526.98360599996522, 3461301.145812),cv::Point(1018526.75805499998387, 3461303.12918899999931),cv::Point(1018526.49988699995447, 3461305.10794499982148),cv::Point(1018526.24169299995992, 3461307.08731799991801),cv::Point(1018525.98381699994206, 3461309.06667699990794)
		,cv::Point(1018525.725449, 3461311.045225),cv::Point(1018525.466354, 3461313.02465099981055),cv::Point(1018525.210591, 3461315.00405699992552),cv::Point(1018524.955531, 3461316.983754),cv::Point(1018524.709184, 3461318.9639699999243)
		,cv::Point(1018524.45971399999689, 3461320.944362),cv::Point(1018524.200573, 3461322.923369),cv::Point(1018523.944644, 3461324.90263599995524),cv::Point(1018523.692914, 3461326.882821) };

    int poly_len = sizeof(in)/sizeof(cv::Point);
 
	vector<cv::Point> in_point(begin(in),end(in));
	
	//n:多项式阶次
	int n = 3;
	cv::Mat mat_k = polyfit(in_point, n);

 
	//计算结果可视化
	// cv::Mat out(150, 500, CV_8UC3, cv::Scalar::all(0));
 
    vector<Point> p_value;
    double y;
	//画出拟合曲线
	for (int i = in[0].x; i > in[ poly_len -1].x; i-=0.1)
	{
        for (int j = 0; j < n + 1; ++j)
		{
			y += mat_k.at<double>(j, 0)*pow(i,j);
		}
        p_value.push_back(Point(i, y, 0));
        
		// cv::Point2d ipt;
		// ipt.x = i;
		// ipt.y = 0;
		// for (int j = 0; j < n + 1; ++j)
		// {
		// 	ipt.y += mat_k.at<double>(j, 0)*pow(i,j);
		// }
		// cv::circle(out, ipt, 1, cv::Scalar(255, 255, 255), CV_FILLED, CV_AA);
	}

    writeShapeFile(p_value);
 
	//画出原始散点
	// for (int i = 0; i < poly_len; ++i)
	// {
	// 	cv::Point ipt = in[i];
	// 	cv::circle(out, ipt, 3, cv::Scalar(0, 0, 255), CV_FILLED, CV_AA);
	// }
 
	// cv::imshow("9次拟合", out);
	// cv::waitKey(0);
	
	return 0;
}



void writeShapeFile(vector<Point>& line_shp)
{
    // 获取shp文件名
    std::string shp_fn = "/home/xiaolinz/uisee_blog/test/3rdparty_test/data/line";
    // 创建对应的.shp文件和.dbf文件
    SHPHandle hShp = SHPCreate(string(shp_fn + ".shp").c_str(), SHPT_ARCZ);
    DBFHandle hDbf = DBFCreate(string(shp_fn + ".dbf").c_str());

    // 创建dbf文件表
    DBFAddField(hDbf, "ID", FTInteger, 10, 0);
    DBFAddField(hDbf, "Name", FTString, 10, 0);
    DBFAddField(hDbf, "Length", FTDouble, 32, 0);

    // dbf的记录数
    int record_idx = DBFGetRecordCount(hDbf);

    // std::vector<Point> line_shp;
    // line_shp.push_back(Point(1.0, 2.0, 3.0));
    // line_shp.push_back(Point(1.5, 2.6, 3.9));

    int nVertices = line_shp.size();
    if (0 == nVertices)
    {
        return;
    }

    double* padfX = new double[nVertices];
    double* padfY = new double[nVertices];
    double* padfZ = new double[nVertices];

    for (int i = 0; i < nVertices; ++i)
    {
        padfX[i] = line_shp.at(i).x;
        padfY[i] = line_shp.at(i).y;
        padfZ[i] = line_shp.at(i).z;
    }

    SHPObject* shpObject = SHPCreateObject(SHPT_ARCZ, -1, 0, NULL, NULL, nVertices, padfX, padfY, padfZ, NULL);
    SHPWriteObject(hShp, -1, shpObject);
    SHPDestroyObject(shpObject);

    delete[] padfX;
    delete[] padfY;
    delete[] padfZ;

    // 字段索引
    int field_idx = 0;

    DBFWriteIntegerAttribute(hDbf, record_idx, field_idx++, 1001);
    DBFWriteStringAttribute(hDbf, record_idx, field_idx++, "polyline");
    DBFWriteDoubleAttribute(hDbf, record_idx, field_idx++, 10.15);

    DBFClose(hDbf);
    SHPClose(hShp);
}

cv::Mat polyfit(vector<cv::Point>& in_point, int n)
{
	int size = in_point.size();
	// 所求未知数个数
	int x_num = n + 1;
	//构造矩阵U和Y
	cv::Mat mat_u(size, x_num, CV_64F);
	cv::Mat mat_y(size, 1, CV_64F);
 
	for (int i = 0; i < mat_u.rows; ++i)
		for (int j = 0; j < mat_u.cols; ++j)
		{
			mat_u.at<double>(i, j) = pow(in_point[i].x, j);
		}
 
	for (int i = 0; i < mat_y.rows; ++i)
	{
		mat_y.at<double>(i, 0) = in_point[i].y;
	}
 
	//矩阵运算，获得系数矩阵K
	cv::Mat mat_k(x_num, 1, CV_64F);
	mat_k = (mat_u.t()*mat_u).inv()*mat_u.t()*mat_y;
	cout << mat_k << endl;
	return mat_k;
}