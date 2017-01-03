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
int m_startorstop = 1;   //�л���Ƶ��ͣ�Ϳ�ʼ
char m_char;    //���ڱ�ǽ���ʽ����
int speed = 31;    //Ĭ�ϲ����ٶ�
Mat frame;
Mat first;
string video;
int number_box = 0;

static CvHaarClassifierCascade* cascade = 0;
static CvMemStorage* storage = 0;

const char* cascade_name ="haarcascade_frontalface_alt.xml";                //�������Ҫ�õ��ķ�����

Rect* detect_and_draw( IplImage* img, int &num )
{

	cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );   //��������������õķ�����
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

	cvCvtColor( img, gray, CV_BGR2GRAY );//cvCvtColor��Opencv�����ɫ�ռ�ת������������ʵ��RGB��ɫ��HSV,HSI����ɫ�ռ��ת����Ҳ����ת��Ϊ�Ҷ�ͼ��
	cvResize( gray, small_img, CV_INTER_LINEAR );//cvResize���µ���ͼ��src��������ROI����ʹ����ȷƥ��Ŀ��dst������ROI����
	cvEqualizeHist( small_img, small_img );//cvEqualizeHist����ʹ�Ҷ�ͼ��ֱ��ͼ���⻯��
	cvClearMemStorage( storage );

	if( cascade )
	{
		/*����cvHaarDetectObjects���ͼ���е�Ŀ�꣬��OpenCV�ṩ��*/     //1.1
		CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage, 1.1, 2, 0 ,cvSize(30, 30) );//cvHaarDetectObjects�������ͼ���е�Ŀ��

		Rect* box = new Rect[faces->total];
		num = faces->total;                   //��⵽������������
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
  cout << "��ѡ�񲥷�ѡ�(ע��1���������ͷ������0����ӱ�����Ƶ����)" ;

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
	  cout << "����Ĳ�����Ҫ�����������룡";
	  goto CHANGE;
	  break;
  }

  cout << "**********************���ܲ˵�**********************" << endl;
  cout << "               �˳� ��        Esc��" << endl;
  cout << "          ��ʼ/��ͣ ��      Space��" << endl;

  if ( change == 0 )
	  {
		  cout << "           ���ٲ��� :        s��S��" << endl;
		  cout << "           �������� :        r��R��" << endl;
		  cout << "           ���ٲ��� ��       l��L��" << endl;
	  }
	  cout << "        Ĭ������°������������Զ����ţ�" << endl;
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
