/*
 * @Description: 
 * @Version: 2.0
 * @Autor: 
 * @Date: 2020-04-20 14:48:45
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2020-04-20 15:35:30
 */


#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>

#include <string>

using namespace std;
using namespace boost::filesystem;

void readFilenamesBoost(vector<string> &filenames, const string folder)
{
  boost::filesystem::path directory(folder);
  boost::filesystem::directory_iterator itr(directory), end_itr;
  string current_file = itr->path().string();
 
  for (; itr != end_itr; ++itr)
  {
    if (is_regular_file(itr->path()))
    {
      string filename = folder + "/" + itr->path().filename().string(); // returns just filename
      filenames.push_back(filename);
      std::cout<<filename<<std::endl;
    }
  }
}

int main(int argc, char *argv[])
{
    string org_dir = "/home/xiaolinz/uisee_data/org_dir/stereo";
    string dst_dir = "/home/xiaolinz/uisee_data/dst_dir";

    std::string fileExtension = ".tiff";

    std::ofstream File;
    
    std::vector<std::string> files;

    readFilenamesBoost(files, org_dir);

    std::string command = "mkdir -p " + dst_dir + "/cam0/";  
    system(command.c_str());

    command = "mkdir -p " + dst_dir + "/cam1/";  
    system(command.c_str());

    for(auto& x: files)
    {
        
        cv::Mat img = cv::imread(x);
        cv::Rect area(0,0,1280,720);
        cv::Mat img_left = img(area);
//        Mat gray_left;
//        cvtColor(img_left, gray_left, CV_BGR2GRAY);
        cv::Rect area2(1280,0,1280,720);
        cv::Mat img_right = img(area2);
//        Mat gray_right;
//        cvtColor(img_right, gray_right, CV_BGR2GRAY);

        

        int bpos=x.find_last_of('/');
        int epos = x.find_last_of('.');
        std::cout<<bpos<<"--"<<epos<<std::endl;
	      string filename(x.substr(bpos+1, epos - bpos - 1));

        string left = dst_dir + "/cam0/" + filename + fileExtension;
        // File.open(left);

        cv::imwrite(left, img_left);

        string right = dst_dir + "/cam1/" + filename + fileExtension;
        // File.open(right);

        cv::imwrite(right, img_right);

    }
}