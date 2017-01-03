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
//Global variables
bool drawing_box = false;
bool tl = false;
bool rep = false;
bool fromfile=false;
int point = 0;  
int m_startorstop = 1;   //切换视频暂停和开始
char m_char;    //用于标记交互式功能
int speed = 31;    //默认播放速度
Mat frame;
Mat first;
string video;
int number_box = 0;

static CvHaarClassifierCascade* cascade = 0;
static CvMemStorage* storage = 0;

const char* cascade_name ="haarcascade_frontalface_alt.xml";                //人脸检测要用到的分类器

Rect* detect_and_draw( IplImage* img, int &num )
{

	cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );   //加载人脸检测所用的分类器
	if( !cascade )
	{
		fprintf( stderr, "ERROR: Could not load classifier cascade\n" );
			exit(1);
	}
	storage = cvCreateMemStorage(0);   

	double scale = 1.3;
	IplImage* gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
	IplImage* small_img = cvCreateImage( cvSize( cvRound (img->width/scale),
		cvRound (img->height/scale)), 8, 1 );

	cvCvtColor( img, gray, CV_BGR2GRAY );//cvCvtColor是Opencv里的颜色空间转换函数，可以实现RGB颜色向HSV,HSI等颜色空间的转换，也可以转换为灰度图像
	cvResize( gray, small_img, CV_INTER_LINEAR );//cvResize重新调整图像src（或它的ROI），使它精确匹配目标dst（或其ROI）。
	cvEqualizeHist( small_img, small_img );//cvEqualizeHist用来使灰度图象直方图均衡化。
	cvClearMemStorage( storage );

	if( cascade )
	{
		/*函数cvHaarDetectObjects检测图像中的目标，由OpenCV提供。*/     //1.1
		CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage, 1.1, 2, 0 ,cvSize(30, 30) );//cvHaarDetectObjects用来检测图像中的目标

		Rect* box = new Rect[faces->total];
		num = faces->total;                   //检测到的总数，返回
		for( int i = 0; i < (faces ? faces->total : 0); i++ )
		{
			CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
			box[i].x = r->x+r->width*0.5;
			box[i].y = r->y+r->height*0.5;
			box[i].height = 1.3*(r->height);
			box[i].width = r->width;
		}
		return box;
	}
}

void print_help(char** argv){
  printf("use:\n     %s -p /path/parameters.yml\n",argv[0]);
  printf("-s    source video\n-b        bounding box file\n-tl  track and learn\n-r     repeat\n");
}

void read_options(int argc, char** argv,VideoCapture& capture,FileStorage &fs){
  for (int i=0;i<argc;i++){
      if (strcmp(argv[i],"-s")==0){
          if (argc>i){
              video = string(argv[i+1]);
              capture.open(video);
              fromfile = false;
          }
          else
            print_help(argv);
		
      }
      if (strcmp(argv[i],"-p")==0){
          if (argc>i){
              fs.open(argv[i+1], FileStorage::READ);
          }
          else
            print_help(argv);
		 
      }
      if (strcmp(argv[i],"-tl")==0){
          tl = true;
      }
      if (strcmp(argv[i],"-r")==0){
          rep = true;
      }
  }
   tl = true;
   rep = true;
}

int main(int argc, char * argv[])
{
 //	./run_tld -p ../parameters.yml -s ../datasets/06_car/car.mpg -tl

  int change;
  cout << "请选择播放选项！(注：1代表从摄像头启动，0代表从本地视频启动)" ;

  CHANGE:
  cin >> change;
  cout << endl;
  switch (change)
  {
  case 0:
	  argc = 6;
	  argv[0] = "./run_tld";
	  argv[1] = "-p";
	  argv[2] = "./parameters.yml";
      argv[3] = "-s";
      argv[4] = "./datasets/06_car/car.mpg";
      argv[5] = "-tl";
	  break;
  case 1:
	  argc = 3;
      argv[0] = "./run_tld";
      argv[1] = "-p";
      argv[2] = "./parameters.yml";
	  break;
  default:
	  cout << "输入的不符合要求！请重新输入！";
	  goto CHANGE;
	  break;
  }

  cout << "**********************功能菜单**********************" << endl;
  cout << "               退出 ：        Esc键" << endl;
  cout << "          开始/暂停 ：      Space键" << endl;

  if ( change == 0 )
	  {
		  cout << "           加速播放 :        s或S键" << endl;
		  cout << "           正常播放 :        r或R键" << endl;
		  cout << "           减速播放 ：       l或L键" << endl;
	  }
	  cout << "        默认情况下按下鼠标左键后自动播放！" << endl;
  cout << "****************************************************" << endl;
  VideoCapture capture;
  capture.open(0);
  FileStorage fs;
  //Read options
  read_options(argc,argv,capture,fs);
  //Init camera
  if (!capture.isOpened())
  {
	cout << "capture device failed to open!" << endl;
    return 1;
  }
  //Register mouse callback to draw the bounding box
  cvNamedWindow("TLD",CV_WINDOW_AUTOSIZE);

  Mat last_gray;

  if (!fromfile)
  {
      if ( !capture.read(frame) )
		  return 0;
      cvtColor(frame, last_gray, CV_RGB2GRAY);
      frame.copyTo(first);
  }
  else
  {
      capture.set(CV_CAP_PROP_FRAME_WIDTH,1024);
      capture.set(CV_CAP_PROP_FRAME_HEIGHT,768);
  }

  FILE  *bb_file = fopen( "bounding_boxes.txt", "w" );

  //Run-time
  
  Mat current_gray_1;
  int frames = 1;
  int detections = 1;
  Rect* rect_box;
  IplImage image;

REPEAT:

  while(1)
  {
	 image = frame;
     rect_box = detect_and_draw(&image, number_box);
	if (!fromfile)
    {
      if ( !capture.read(frame) )
		  return 0;
    }
    else
	{
      first.copyTo(frame);
	}
	cvtColor(frame, current_gray_1, CV_RGB2GRAY);

	if ( number_box > 0 )
	{
		TLD *tld = new TLD[number_box];
		vector<Point2f> *pts1 = new vector<Point2f>[number_box];
		vector<Point2f> *pts2 = new vector<Point2f>[number_box];
		BoundingBox *pbox = new BoundingBox[number_box];
		bool *status = new bool[number_box];
		for ( int i = 0; i < number_box; i++ )
			status[i] = true;
		if ( point == 0 )
		{
			for ( int i = 0; i < number_box; i++ )
			{
				tld[i].read(fs.getFirstTopLevelNode());
				tld[i].init(last_gray,rect_box[i],bb_file);
			}
		}
		while ( 1 )
		{
			for ( int i = 0; i < number_box; i++ )
			{
				tld[i].processFrame(last_gray,current_gray_1,pts1[i],pts2[i],pbox[i],status[i],tl,bb_file);
				if (status[i])
				{
					drawPoints(frame,pts1[i],Scalar(220,200,100));
					drawPoints(frame,pts2[i],Scalar(0,0,255));
					drawBox(frame,pbox[i]);
					//detections++;
				}
				imshow("TLD", frame);
				swap(last_gray,current_gray_1);
				for ( int i = 0; i < number_box; i++ )
				{
					pts1[i].clear();
				   pts2[i].clear();
				}
			}
			if (!fromfile)
            {
				if ( !capture.read(frame) )
				   return 0;
			}
			else
			{
				first.copyTo(frame);
			}
			cvtColor(frame, current_gray_1, CV_RGB2GRAY);

			m_char = cvWaitKey(speed);
			switch (m_char)
			{
			case 27:
				return 0;
				break;
			case ' ':
				if ( ++m_startorstop % 2 == 1 )
				{
					fromfile = false;
					frame.copyTo(first);
				}
				else
					fromfile = true;
				break;
			case 's':
			case 'S':
				if (speed >= 6 )
					speed -= 5;
				break;
			case 'l':
			case 'L':
				if (speed <= 181)
					speed += 5;
				break;
			case 'r':
			case 'R':
				speed = 31;
				break;
			default:
				break;
			}
		}
	}
	
    //Draw Points
    
	if ( number_box == 0 )
	   imshow("TLD", frame);
    
	m_char = cvWaitKey(speed);
    switch (m_char)
	{
	case 27:
		return 0;
		break;
	case ' ':
		if ( ++m_startorstop % 2 == 1 )
		{
			fromfile = false;
			frame.copyTo(first);
		}
		else
			fromfile = true;
		break;
	case 's':
	case 'S':
		if (speed >= 6 )
			speed -= 5;
		break;
	case 'l':
	case 'L':
		if (speed <= 181)
			speed += 5;
		break;
	case 'r':
	case 'R':
		speed = 31;
		break;
	default:
		break;
	}
	
  }
  if (rep)
  {
    rep = false;
    tl = false;
    fclose(bb_file);
    bb_file = fopen("final_detector.txt","w");
    //capture.set(CV_CAP_PROP_POS_AVI_RATIO,0);
    capture.release();
    capture.open(video);
    goto REPEAT;
  }
  fclose(bb_file);
  return 0;
 }
