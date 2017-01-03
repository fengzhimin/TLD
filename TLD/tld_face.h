#include <opencv2/opencv.hpp>
#include "tld_utils.h"
#include <iostream>
#include <sstream>
#include "TLD.h"
#include <stdio.h>
#include <Windows.h>
#include "cv.h"
#include "highgui.h"

using namespace cv;
using namespace std;
//参数capture为打开摄像头的指针
//参数namewindow为已经打开的视频窗口名字
//参数rect_box为存放跟踪物体的初始坐标
//参数number_box为跟踪物体的个数，默认情况下为0
int Tld_face(VideoCapture* capture, string *namewindow, Rect* rect_box, int number_box = 0)
{
	FileStorage fs;
	fs.open("./parameters.yml", FileStorage::READ);
	bool tl = true;
    bool rep = true;
	char m_char;
	Mat frame;
	int frames = 1;
    int detections = 1;

	FILE  *bb_file = fopen( "bounding_boxes.txt", "w" );
   
    while(1)
    {
		if ( number_box < 0 )
		{
			MessageBox(NULL, L"参数number_box必须是一个大于等于0的整数！", L"错误提示" ,MB_OK );
			return 1;
		}
		if ( !(*capture).read(frame) )
			return 0;
		
		if ( number_box > 0 )
		{
			TLD *tld = new TLD[number_box];
			vector<Point2f> *pts1 = new vector<Point2f>[number_box];
			vector<Point2f> *pts2 = new vector<Point2f>[number_box];
			BoundingBox *pbox = new BoundingBox[number_box];
			Mat *last_gray = new Mat[number_box];
			Mat *current_gray = new Mat[number_box];
			bool *status = new bool[number_box];

			for ( int i = 0; i < number_box; i++ )
			{
				cvtColor(frame, last_gray[i], CV_RGB2GRAY);
				cvtColor(frame, current_gray[i], CV_RGB2GRAY);
				status[i] = true;
			}

			for ( int i = 0; i < number_box; i++ )
			{
				tld[i].read(fs.getFirstTopLevelNode());
				tld[i].init(last_gray[i],rect_box[i],bb_file);
			}
			
			while ( 1 )
			{

				for ( int i = 0; i < number_box; i++ )
				{
					tld[i].processFrame(last_gray[i],current_gray[i],pts1[i],pts2[i],pbox[i],status[i],tl,bb_file);
					if (status[i])
					{
						drawPoints(frame,pts1[i],Scalar(220,200,100));
						drawPoints(frame,pts2[i],Scalar(0,0,255));
						drawBox(frame,pbox[i]);
						//detections++;
					}
					
				}

				imshow((*namewindow), frame);
				for ( int i = 0; i < number_box; i++ )
				{
					swap(last_gray[i],current_gray[i]);
					pts1[i].clear();
					pts2[i].clear();
				}

				if ( !(*capture).read(frame) )
				   return 0;
				for ( int i = 0; i < number_box; i++ )
				    cvtColor(frame, current_gray[i], CV_RGB2GRAY);

				m_char = cvWaitKey(11);
				switch (m_char)
				{
				case 27:
					return 0;
					break;
				default:
					break;
				}
			}
		}
		
		imshow((*namewindow), frame);
    
		m_char = cvWaitKey(33);
		switch (m_char)
		{
		case 27:
			return 0;
			break;
		default:
			break;
		}
    }
}

