#include <opencv2/opencv.hpp>
#include "tld_utils.h"
#include <iostream>
#include <sstream>
#include "TLD.h"
#include <stdio.h>
using namespace cv;
using namespace std;
//Global variables
Rect box;
Rect box_2;
bool drawing_box = false;
bool gotBB = false;
bool tl = false;
bool rep = false;
bool fromfile=false;
int point = 0;   //切换跟踪目标
int m_startorstop = 1;   //切换视频暂停和开始
char m_char;    //用于标记交互式功能
int speed = 31;    //默认播放速度
Mat frame;
Mat first;
string video;

void readBB(char* file){
  ifstream bb_file (file);
  string line;
  getline(bb_file,line);
  istringstream linestream(line);
  string x1,y1,x2,y2;
  getline (linestream,x1, ',');
  getline (linestream,y1, ',');
  getline (linestream,x2, ',');
  getline (linestream,y2, ',');
  int x = atoi(x1.c_str());// = (int)file["bb_x"];
  int y = atoi(y1.c_str());// = (int)file["bb_y"];
  int w = atoi(x2.c_str())-x;// = (int)file["bb_w"];
  int h = atoi(y2.c_str())-y;// = (int)file["bb_h"];
  box = Rect(x,y,w,h);
}

//bounding box mouse callback
void mouseHandler(int event, int x, int y, int flags, void *param){
  switch( event ){
  case CV_EVENT_MOUSEMOVE:
    if (drawing_box){
        box.width = x-box.x;
        box.height = y-box.y;
    }
    break;
  case CV_EVENT_LBUTTONDOWN:
    drawing_box = true;

	if ( point == 0 )
		point = 1;
	fromfile = false;
    frame.copyTo(first);

    box = Rect( x, y, 0, 0 );
    break;
  case CV_EVENT_LBUTTONUP:
    drawing_box = false;
	point = 0;
    if( box.width < 0 ){
        box.x += box.width;
        box.width *= -1;
    }
    if( box.height < 0 ){
        box.y += box.height;
        box.height *= -1;
    }
    gotBB = true;
	fromfile = true;
	if (m_startorstop % 2 == 1 )
		m_startorstop++;
    break;
  }
}

void print_help(char** argv){
  printf("use:\n     %s -p /path/parameters.yml\n",argv[0]);
  printf("-s    source video\n-b        bounding box file\n-tl  track and learn\n-r     repeat\n");
}

void read_options(int argc, char** argv,VideoCapture& capture,FileStorage &fs){
  for (int i=0;i<argc;i++){
      if (strcmp(argv[i],"-b")==0){
          if (argc>i){
              readBB(argv[i+1]);
              gotBB = true;

          }
          else
            print_help(argv);
		   
      }
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
  argc = 6;
  argv[0] = "./run_tld";
  argv[1] = "-p";
  argv[2] = "./parameters.yml";
  argv[3] = "-s";
  argv[4] = "./datasets/06_car/car.mpg";
  argv[5] = "-tl";

  cout << "**********************功能菜单**********************" << endl;
  cout << "               退出 ：        Esc键" << endl;
  cout << "          开始/暂停 ：      Space键" << endl;
  for ( int i = 1; i < argc; i++ )
	  if ( argv[i] == "-s" )
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
  cvSetMouseCallback( "TLD", mouseHandler, NULL );
  //TLD framework
  TLD tld_1, tld_2;
  //Read parameters file
  tld_1.read(fs.getFirstTopLevelNode());
  tld_2.read(fs.getFirstTopLevelNode());

  Mat last_gray, last_gray_2;
 
  if (!fromfile)
  {
      if ( !capture.read(frame) )
		  return 0;
      cvtColor(frame, last_gray, CV_RGB2GRAY);
	  cvtColor(frame, last_gray_2, CV_RGB2GRAY);
      frame.copyTo(first);
  }
  else
  {
      capture.set(CV_CAP_PROP_FRAME_WIDTH,1024);
      capture.set(CV_CAP_PROP_FRAME_HEIGHT,768);
  }
  ///Initialization
GETBOUNDINGBOX:
  while(!gotBB)
  {
    if (fromfile)
	{
      if ( !capture.read(frame) )
		 return 0;
    }
    else
	{
      first.copyTo(frame);
	}

    cvtColor(frame, last_gray, CV_RGB2GRAY);
	cvtColor(frame, last_gray_2, CV_RGB2GRAY);
	//画跟踪窗口
    drawBox(frame,box);
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

  if (min(box.width,box.height)<(int)fs.getFirstTopLevelNode()["min_win"])
  {
      cout << "Bounding box too small, try again." << endl;
      gotBB = false;
      goto GETBOUNDINGBOX;
  }
  //响应鼠标事件
  //cvSetMouseCallback( "TLD", NULL, NULL );
  printf("Initial Bounding Box = x:%d y:%d h:%d w:%d\n",box.x,box.y,box.width,box.height);
  //Output file
  FILE  *bb_file = fopen("bounding_boxes.txt","w");
  FILE  *bb_file_2 = fopen("bounding_boxes_2.txt","w");
  //TLD initialization
  //确定下一帧的位置，并且将其写入bounding_boxes.txt文件中
  tld_1.init(last_gray,box,bb_file);
  box_2 = Rect(100,80,50,40);
  tld_2.init(last_gray_2,box_2,bb_file);
  //Run-time
  cout << " I am test1 " << endl;
#ifdef FIRST
  status_1=true;
  status_2=true;
  frames = 1;
  detections = 1;
#endif

#ifndef FIRST
#define FIRST
  Mat current_gray_1;
  BoundingBox pbox_1;
  Mat current_gray_2;
  BoundingBox pbox_2;
  vector<Point2f> pts1_1;
  vector<Point2f> pts1_2;
  vector<Point2f> pts2_1;
  vector<Point2f> pts2_2;
  bool status_1=true;
  bool status_2=true;
  int frames = 1;
  int detections = 1;
#endif

REPEAT:
  while(1)
  {

   if (fromfile)
   {
      if ( !capture.read(frame) )
		  return 0;
    }
    else
	{
      first.copyTo(frame);
	}
    //get frame
    cvtColor(frame, current_gray_1, CV_RGB2GRAY);
	cvtColor(frame, current_gray_2, CV_RGB2GRAY);
	cout << " I am test2 " << endl;
    //Process Frame
    tld_1.processFrame(last_gray,current_gray_1,pts1_1,pts1_2,pbox_1,status_1,tl,bb_file);
	cout << " I am test3 " << endl;
	tld_2.processFrame(last_gray_2,current_gray_2,pts2_1,pts2_2,pbox_2,status_2,tl,bb_file_2);
	cout << " I am test4 " << endl;
    //Draw Points
    if (status_1)
	{
      drawPoints(frame,pts1_1);
      drawPoints(frame,pts1_2,Scalar(0,255,0));
      drawBox(frame,pbox_1);
      //detections++;
    }

	if (status_2)
	{
      drawPoints(frame,pts2_1);
      drawPoints(frame,pts2_2,Scalar(0,255,0));
      drawBox(frame,pbox_2);
      //detections++;
    }

    //Display
    imshow("TLD", frame);
    //swap points and images
    swap(last_gray,current_gray_1);
    pts1_1.clear();
    pts1_2.clear();

	swap(last_gray_2,current_gray_2);
    pts2_1.clear();
    pts2_2.clear();

    //frames++;
    //printf("Detection rate: %d/%d\n",detections,frames);
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
	/*
	if ( point == 1 ) 
	{
		point = 0;
		gotBB = false;
		fclose(bb_file);
		goto GETBOUNDINGBOX;
	}
	*/
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
