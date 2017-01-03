#include <opencv2/opencv.hpp>
#include "tld_utils.h"
#include <iostream>
#include <sstream>
#include "TLD.h"
#include <stdio.h>
#include <Windows.h>
#include "opencv2/video/background_segm.hpp"
#include "opencv2/legacy/blobtrack.hpp"
#include "opencv2/legacy/legacy.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/imgproc/imgproc_c.h>
#include "opencv2/features2d/features2d.hpp"
#include "cv.h"
#include "highgui.h"

using namespace cv;
using namespace std;

//Global variables
char choose;     //进入不同的功能选项 0代表进入手动跟踪模式， 1代表进入自动跟踪模式  2代表进入自动人脸跟踪模式
bool drawing_box = false;
bool gotBB = false;
bool tl = false;
bool rep = false;
bool fromfile=false;
int point = 0;   //切换跟踪目标
bool m_point = false;
bool m_point1 = false;
int m_startorstop = 1;   //切换视频暂停和开始
char m_char;    //用于标记交互式功能
int speed = 31;    //默认播放速度
Mat frame;
Mat first;
string video;
Rect box;

/*******************************Hander*****************************/
/*手动跟踪物体*/
/******************************************************************/

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
	fromfile = false;
    frame.copyTo(first);

    box = Rect( x, y, 0, 0 );
    break;
  case CV_EVENT_LBUTTONUP:
    drawing_box = false;
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
	if ( point == 6 )
	{
		cvSetMouseCallback( "TLD", NULL, NULL );
	}
	++point;
	m_point = m_point1 = true;
	
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
      if (strcmp(argv[i],"-b")==0)
	  {
          if (argc>i)
		  {
              readBB(argv[i+1]);
              gotBB = true;

          }
          else
            print_help(argv);
		   
      }
      if (strcmp(argv[i],"-s")==0)
	  {
          if (argc>i)
		  {
              video = string(argv[i+1]);
              capture.open(video);
              fromfile = false;
          }
          else
            print_help(argv);
		
      }
      if (strcmp(argv[i],"-p")==0)
	  {
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
int Hander( int argc, char **argv )
{
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
		  cout << "         标记跟踪物 ：     鼠标左键" << endl;
		   cout << "         删除跟踪物：       d或D键" << endl;
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
			return 0;
		  }
		  //Register mouse callback to draw the bounding box
		  cvNamedWindow("TLD",CV_WINDOW_AUTOSIZE);
		  cvSetMouseCallback( "TLD", mouseHandler, NULL );
		  //TLD framework
		  TLD tld[6];
		  //Read parameters file
		  for ( int i = 0; i < 6; ++i )
			 tld[i].read(fs.getFirstTopLevelNode());
 
		  Mat last_gray;
 
		  if (!fromfile)
		  {
			  if ( !capture.read(frame) )
			  {
				  ReleaseCapture();
				  cvDestroyWindow("TLD");
				  return 0;
			  }
			  cvtColor(frame, last_gray, CV_RGB2GRAY);
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
			  {
				  ReleaseCapture();
				  cvDestroyWindow("TLD");
				 return 0;
			  }
			}
			else
			{
			  first.copyTo(frame);
			}

			cvtColor(frame, last_gray, CV_RGB2GRAY);
			//画跟踪窗口
			drawBox( frame,box,Scalar(0, 0 , 255) );
			imshow("TLD", frame);

			m_char = cvWaitKey(speed);
			switch (m_char)
			{
			case 27:
				cvDestroyWindow("TLD");
				ReleaseCapture();
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
			  MessageBox(NULL,L"窗口太小！请重新绘制！", L"提示", MB_OK);
			  gotBB = false;
			  point--;
			  box = Rect(0,0,0,0);
			  goto GETBOUNDINGBOX;
		  }

		  //响应鼠标事件
		  //cvSetMouseCallback( "TLD", NULL, NULL );
		  printf("Initial Bounding Box = x:%d y:%d h:%d w:%d\n",box.x,box.y,box.width,box.height);
  
		#ifndef DELETE_FIRST_OBJECT
		  //Output file
		  FILE  *bb_file = fopen( "bounding_boxes.txt", "w" );

		  //Run-time
  
		  Mat current_gray_1;
		  BoundingBox pbox[6];
		  vector<Point2f> pts1[6];
		  vector<Point2f> pts2[6];
		  bool status[6] = {true};
		  int frames = 1;
		  int detections = 1;

		#endif

		  //TLD initialization
		  //确定下一帧的位置，并且将其写入bounding_boxes.txt文件中
		  tld[0].init(last_gray,box,bb_file);

		REPEAT:

		  while(1)
		  {
			if ( m_point1 == true )
			{
			  if (min(box.width, box.height)<(int)fs.getFirstTopLevelNode()["min_win"])
			  {
				 gotBB = false;
				 if ( point-- == 6 )
					 cvSetMouseCallback( "TLD", mouseHandler, NULL );
				 MessageBox(NULL,L"窗口太小！请重新绘制！", L"提示", MB_OK);
			  }
			  else
			  {
				  tld[point - 1].init(last_gray, box, bb_file);
			  }
			  m_point1 = false;
			}
		   if (fromfile)
		   {
			  if ( !capture.read(frame) )
			  {
				  cvDestroyWindow("TLD");
				  ReleaseCapture();
				  return 0;
			  }
		   }
			else
			{
			  first.copyTo(frame);
			}

			//get frame
			cvtColor(frame, current_gray_1, CV_RGB2GRAY);
			//Process Frame
			//Draw Points
			for ( int i = 0; i < point; ++i )
			{
				tld[i].processFrame(last_gray,current_gray_1,pts1[i],pts2[i],pbox[i],status[i],tl,bb_file);

				if (status[i])
				{
				  drawPoints(frame,pts1[i],Scalar(220,200,100));
				  drawPoints(frame,pts2[i],Scalar(0,0,255));
				  drawBox(frame,pbox[i],Scalar(255,0,0));
				  //detections++;
				}
			}

			if ( point >= 1 && drawing_box == true )
			{
				drawBox( frame,box,Scalar(0, 0 , 255) );
			}
			//Display
			imshow("TLD", frame);
			//swap points and images
			swap(last_gray,current_gray_1);
    
			for ( int i = 0; i < point; i++ )
			{
				pts1[i].clear();
				pts2[i].clear();
			}

		   // frames++;
		   // printf("Detection rate: %d/%d\n",detections,frames);
			m_char = cvWaitKey(speed);
			switch (m_char)
			{
			case 27:
				cvDestroyWindow("TLD");
				ReleaseCapture();
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
			case 'd':
			case 'D':
				if ( point-- == 6 )
					 cvSetMouseCallback( "TLD", mouseHandler, NULL );
				if ( point == 0 )
				{
		#define DELETE_FIRST_OBJECT
					gotBB = false;
					box = Rect(0,0,0,0);
					pts1[point].clear();
					pts2[point].clear();
					pbox[point] = box;
					goto GETBOUNDINGBOX;
				}
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
}
/******************************Hander*********************************/


/*****************************Auto*************************************/
/*自动跟踪俩个物体*/
/**********************************************************************/

/* Select appropriate case insensitive string comparison function: */
#if defined WIN32 || defined _MSC_VER
# define MY_STRNICMP _strnicmp
# define MY_STRICMP _stricmp
# define MY_STRDUP _strdup
#else
# define MY_STRNICMP strncasecmp
# define MY_STRICMP strcasecmp
# define MY_STRDUP strdup
#endif

/* List of foreground (FG) DETECTION modules: */
static CvFGDetector* cvCreateFGDetector0() { return cvCreateFGDetectorBase(CV_BG_MODEL_FGD, NULL); }
static CvFGDetector* cvCreateFGDetector0Simple() { return cvCreateFGDetectorBase(CV_BG_MODEL_FGD_SIMPLE, NULL); }
static CvFGDetector* cvCreateFGDetector1() { return cvCreateFGDetectorBase(CV_BG_MODEL_MOG, NULL); }

typedef struct DefModule_FGDetector
{
	CvFGDetector* (*create)();
	const char* nickname;
	const char* description;
} DefModule_FGDetector;

DefModule_FGDetector FGDetector_Modules[] =
{
	{ cvCreateFGDetector0, "FG_0", "Foreground Object Detection from Videos Containing Complex Background. ACM MM2003." },
	{ cvCreateFGDetector0Simple, "FG_0S", "Simplified version of FG_0" },
	{ cvCreateFGDetector1, "FG_1", "Adaptive background mixture models for real-time tracking. CVPR1999" },
	{ NULL, NULL, NULL }
};

/* List of BLOB DETECTION modules: */
typedef struct DefModule_BlobDetector
{
	CvBlobDetector* (*create)();
	const char* nickname;
	const char* description;
} DefModule_BlobDetector;

DefModule_BlobDetector BlobDetector_Modules[] =
{
	{ cvCreateBlobDetectorCC, "BD_CC", "Detect new blob by tracking CC of FG mask" },
	{ cvCreateBlobDetectorSimple, "BD_Simple", "Detect new blob by uniform moving of connected components of FG mask" },
	{ NULL, NULL, NULL }
};

/* List of BLOB TRACKING modules: */
typedef struct DefModule_BlobTracker
{
	CvBlobTracker* (*create)();
	const char* nickname;
	const char* description;
} DefModule_BlobTracker;

DefModule_BlobTracker BlobTracker_Modules[] =
{
	{ cvCreateBlobTrackerCCMSPF, "CCMSPF", "connected component tracking and MSPF resolver for collision" },
	{ cvCreateBlobTrackerCC, "CC", "Simple connected component tracking" },
	{ cvCreateBlobTrackerMS, "MS", "Mean shift algorithm " },
	{ cvCreateBlobTrackerMSFG, "MSFG", "Mean shift algorithm with FG mask using" },
	{ cvCreateBlobTrackerMSPF, "MSPF", "Particle filtering based on MS weight" },
	{ NULL, NULL, NULL }
};

/* List of Blob Trajectory ANALYSIS modules: */
/*================= END MODULES DECRIPTION ===================================*/

/* Run pipeline on all frames: */
static Rect RunBlobTrackingAuto(CvCapture* pCap, CvBlobTrackerAuto* pTracker, char* fgavi_name = NULL, char* btavi_name = NULL)
{
	int                     OneFrameProcess = 0;
	int                     key;
	int                     FrameNum = 0;
	CvVideoWriter*          pFGAvi = NULL;
	CvVideoWriter*          pBTAvi = NULL;

	Rect R ;

	/* Main loop: */
	for (FrameNum = 0; pCap && (key = cvWaitKey(OneFrameProcess ? 0 : 1)) != 27;
		FrameNum++)
	{   /* Main loop: */
		IplImage*   pImg = NULL;
		IplImage*   pMask = NULL;

		if (key != -1)
		{
			OneFrameProcess = 1;
			if (key == 'r')OneFrameProcess = 0;
		}

		pImg = cvQueryFrame(pCap);
	
		if (pImg == NULL) break;


		/* Process: */
		pTracker->Process(pImg, pMask);

	/* Draw debug info: */
		if (pImg)
		{   /* Draw all information about test sequence: */
			char        str[1024];
			int         line_type = CV_AA;   // Change it to 8 to see non-antialiased graphics.
			CvFont      font;
			int         i;
			IplImage*   pI = cvCloneImage(pImg);

			cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 0.7, 0.7, 0, 1, line_type);
		
			for (i = pTracker->GetBlobNum(); i> 0; i--)
			{
				CvSize  TextSize;
				CvBlob* pB = pTracker->GetBlob(i - 1);
				CvPoint p = cvPoint(cvRound(pB->x * 256), cvRound(pB->y * 256));
				CvSize  s = cvSize(MAX(1, cvRound(CV_BLOB_RX(pB) * 256)), MAX(1, cvRound(CV_BLOB_RY(pB) * 256)));
				int c = cvRound(255 * pTracker->GetState(CV_BLOB_ID(pB)));

				cvEllipse(pI,
					p,
					s,
					0, 0, 360,
					CV_RGB(c, 255 - c, 0), cvRound(1 + (3 * 0) / 255), CV_AA, 8);
				p.x >>= 8;
				p.y >>= 8;
				s.width >>= 8; 
				s.height >>= 8;
				R = Rect(p.x - s.width - 14, p.y - s.height + 3, s.width * 2 - 5, s.height * 2);
				if ((R.width != 0) && (R.height != 0))
				{
					return R;
				}

			}   /* Next blob. */;

			cvReleaseImage(&pI);
		}   /* Draw all information about test sequence. */
    }   /*  Main loop. */

	if (pFGAvi)cvReleaseVideoWriter(&pFGAvi);
	if (pBTAvi)cvReleaseVideoWriter(&pBTAvi);
	//return box1;
}   /* RunBlobTrackingAuto */

/* Read parameters from command line
* and transfer to specified module:
*/
static void set_params(int argc, char* argv[], CvVSModule* pM, const char* prefix, const char* module)
{
	int prefix_len = (int)strlen(prefix);
	int i;
	for (i = 0; i<argc; ++i)
	{
		int j;
		char* ptr_eq = NULL;
		int   cmd_param_len = 0;
		char* cmd = argv[i];
		if (MY_STRNICMP(prefix, cmd, prefix_len) != 0) continue;
		cmd += prefix_len;
		if (cmd[0] != ':')continue;
		cmd++;

		ptr_eq = strchr(cmd, '=');
		if (ptr_eq)
			cmd_param_len = (int)(ptr_eq - cmd);

		for (j = 0;; ++j)
		{
			int     param_len;
			const char*   param = pM->GetParamName(j);
			if (param == NULL) break;
			param_len = (int)strlen(param);
			if (cmd_param_len != param_len) continue;
			if (MY_STRNICMP(param, cmd, param_len) != 0) continue;
			cmd += param_len;
			if (cmd[0] != '=')continue;
			cmd++;
			pM->SetParamStr(param, cmd);
			printf("%s:%s param set to %g\n", module, param, pM->GetParam(param));
		}
	}

	pM->ParamUpdate();

}

int Auto( int argc, char** argv )
{
	argc = 11;
	argv[1] = "fg = FG_0s";
	argv[2] = "bd:hmin = 0.08";
	argv[3] = "bt = CCMSPF";
	argv[4] = "btpp = Kalman";
	argv[5] = "bt_corr = PostProcRes";
	argv[6] = "btgen = YML";
	argv[7] = "track = id021301.txt";
	argv[8] = "btavi = test_021301.avi";
	argv[9] = "fgavi = f021301.avi";
	argv[10] = "./11.avi";

	CvCapture*                  pCap = NULL;
	CvBlobTrackerAutoParam1     param = { 0, 0, 0, 0, 0, 0, 0, 0 };
	CvBlobTrackerAuto*          pTracker = NULL;

	//float       scale = 1;
	const char* scale_name = NULL;
	char*       yml_name = NULL;
	char**      yml_video_names = NULL;
	int         yml_video_num = 0;
	char*       avi_name = NULL;
	const char* fg_name = NULL;
	char*       fgavi_name = NULL;
	char*       btavi_name = NULL;
	const char* bd_name = NULL;
	const char* bt_name = NULL;
	const char* btgen_name = NULL;
	const char* btpp_name = NULL;
	const char* bta_name = NULL;
	char*       bta_data_name = NULL;
	char*       track_name = NULL;
	//char*       comment_name = NULL;
	char*       FGTrainFrames = NULL;
	char*       log_name = NULL;
	char*       savestate_name = NULL;
	char*       loadstate_name = NULL;
	const char* bt_corr = NULL;
	
	DefModule_FGDetector*           pFGModule = NULL;
	DefModule_BlobDetector*         pBDModule = NULL;
	DefModule_BlobTracker*          pBTModule = NULL;

	cvInitSystem(argc, argv);
	{   /* Parse arguments: */
		int i;
		for (i = 1; i<argc; ++i)
		{
			int bParsed = 0;
			size_t len = strlen(argv[i]);
#define RO(_n1,_n2) if(strncmp(argv[i],_n1,strlen(_n1))==0) {_n2 = argv[i]+strlen(_n1);bParsed=1;};
			RO("fg=", fg_name);
			RO("fgavi=", fgavi_name);
			RO("btavi=", btavi_name);
			RO("bd=", bd_name);
			RO("bt=", bt_name);
			RO("bt_corr=", bt_corr);
			RO("btpp=", btpp_name);
			RO("bta=", bta_name);
			RO("bta_data=", bta_data_name);
			RO("btgen=", btgen_name);
			RO("track=", track_name);
			//RO("comment=",comment_name);
			RO("FGTrainFrames=", FGTrainFrames);
			RO("log=", log_name);
			RO("savestate=", savestate_name);
			RO("loadstate=", loadstate_name);
#undef RO
			{
				char* ext = argv[i] + len - 4;
				if (strrchr(argv[i], '=') == NULL &&
					!bParsed &&
					(len>3 && (MY_STRICMP(ext, ".avi") == 0)))
				{
					avi_name = argv[i];
					break;
				}
			}   /* Next argument. */
		}
	}   /* Parse arguments. */

	

	{   /* Set default parameters for one processing: */
		//if (!bt_corr) bt_corr = "none";
		if (!fg_name) fg_name = FGDetector_Modules[0].nickname;
		if (!bd_name) bd_name = BlobDetector_Modules[0].nickname;
		if (!bt_name) bt_name = BlobTracker_Modules[0].nickname;
		if (!scale_name) scale_name = "1";
	}

	for (pFGModule = FGDetector_Modules; pFGModule->nickname; ++pFGModule)
	if (fg_name && MY_STRICMP(fg_name, pFGModule->nickname) == 0) break;

	for (pBDModule = BlobDetector_Modules; pBDModule->nickname; ++pBDModule)
	if (bd_name && MY_STRICMP(bd_name, pBDModule->nickname) == 0) break;
	
	for (pBTModule = BlobTracker_Modules; pBTModule->nickname; ++pBTModule)
	if (bt_name && MY_STRICMP(bt_name, pBTModule->nickname) == 0) break;
	
	/* Create source video: */
	if (avi_name)
		pCap = cvCaptureFromFile(avi_name);/*pCap = cvCaptureFromFile(avi_name);*/
	    

	if (pCap == NULL)
	{
		printf("Can't open %s file\n", avi_name);
		return 1;
	}

	{   /* Create autotracker module and its components: */
		param.FGTrainFrames = FGTrainFrames ? atoi(FGTrainFrames) : 0;

		/* Create FG Detection module: */
		param.pFG = pFGModule->create();
		if (!param.pFG)
			puts("Can not create FGDetector module");
		param.pFG->SetNickName(pFGModule->nickname);
		set_params(argc, argv, param.pFG, "fg", pFGModule->nickname);

		/* Create Blob Entrance Detection module: */
		param.pBD = pBDModule->create();
		if (!param.pBD)
			puts("Can not create BlobDetector module");
		param.pBD->SetNickName(pBDModule->nickname);
		set_params(argc, argv, param.pBD, "bd", pBDModule->nickname);

		/* Create blob tracker module: */
		param.pBT = pBTModule->create();
		if (!param.pBT)
			puts("Can not create BlobTracker module");
		param.pBT->SetNickName(pBTModule->nickname);
		set_params(argc, argv, param.pBT, "bt", pBTModule->nickname);

		/* Create whole pipline: */
		pTracker = cvCreateBlobTrackerAuto1(&param);
		if (!pTracker)
			puts("Can not create BlobTrackerAuto");
	}

	/* Run pipeline: */
	
	box=RunBlobTrackingAuto(pCap, pTracker, fgavi_name, btavi_name);

	cvNamedWindow("TLD1", 0);
	FileStorage fs;
	//Read option、s
	fs.open("./parameters.yml", FileStorage::READ);

	TLD tld;
	//Read parameters file
	tld.read(fs.getFirstTopLevelNode());
	Mat frame;
	//IplImage* pImg = &frame.operator IplImage();
	Mat last_gray;
	Mat first;
	if (fromfile){
		frame = cvQueryFrame(pCap);
		cvtColor(frame, last_gray, CV_RGB2GRAY);
		frame.copyTo(first);
	}
	else{
		//capture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
		//capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	}

	///Initialization

GETBOUNDINGBOX:

	while (!gotBB)
	{
		if (!fromfile){
			frame = cvQueryFrame(pCap);
		}
		else
			first.copyTo(frame);
		cvtColor(frame, last_gray, CV_RGB2GRAY);
		gotBB = true;
		//box = box1;
		drawBox(frame, box);

		imshow("TLD1", frame);
		if (cvWaitKey(33) == 27)
		{
			cvDestroyWindow("TLD1");
			ReleaseCapture();
			return 1;
		}
	}
	if (min(box.width, box.height)<(int)fs.getFirstTopLevelNode()["min_win"]){
		cout << "Bounding box too small, try again." << endl;
		gotBB = false;
		goto GETBOUNDINGBOX;
	}
	//Remove callback
	printf("Initial Bounding Box = x:%d y:%d h:%d w:%d\n", box.x, box.y, box.width, box.height);
	//Output file
	FILE  *bb_file = fopen("bounding_boxes.txt", "w");
	//TLD initialization
	tld.init(last_gray, box, bb_file);

	///Run-time
	Mat current_gray;
	BoundingBox pbox;
	vector<Point2f> pts1;
	vector<Point2f> pts2;
	bool status = true;
	int frames = 1;
	int detections = 1;
REPEAT:
	while (cvQueryFrame(pCap)){
		//get frame
		cvtColor(frame, current_gray, CV_RGB2GRAY);
		//Process Frame
		tld.processFrame(last_gray, current_gray, pts1, pts2, pbox, status, tl, bb_file);
		//Draw Points
		if (status){
			drawPoints(frame, pts1);
			drawPoints(frame, pts2, Scalar(0, 255, 0));
			drawBox(frame, pbox);
			detections++;
		}
		//Display
		//IplImage* pframe = &frame.operator IplImage();
		//cvShowImage("TLD", pframe);
		imshow("TLD1", frame);
		//swap points and images
		swap(last_gray, current_gray);
		pts1.clear();
		pts2.clear();
		frames++;
		printf("Detection rate: %d/%d\n", detections, frames);
		if (cvWaitKey(33) == 27)
			break;
	}
	if (rep){
		rep = false;
		tl = false;
		fclose(bb_file);
		bb_file = fopen("final_detector.txt", "w");
		//capture.set(CV_CAP_PROP_POS_AVI_RATIO,0);
		//capture.release();
		//capture.open(video);
		goto REPEAT;
	}
	fclose(bb_file);
	{   /* Save state and release modules: */
		CvFileStorage* fs = NULL;
		if (savestate_name)
		{
			fs = cvOpenFileStorage(savestate_name, NULL, CV_STORAGE_WRITE);
		}
		if (fs)
		{
			cvStartWriteStruct(fs, "BlobTracker", CV_NODE_MAP);
			if (param.pBT)param.pBT->SaveState(fs);
			cvEndWriteStruct(fs);
			cvStartWriteStruct(fs, "BlobTrackerAuto", CV_NODE_MAP);
			if (pTracker)pTracker->SaveState(fs);
			cvEndWriteStruct(fs);
			cvStartWriteStruct(fs, "BlobTrackAnalyser", CV_NODE_MAP);
			if (param.pBTA)param.pBTA->SaveState(fs);
			cvEndWriteStruct(fs);
			cvReleaseFileStorage(&fs);
		}
		if (param.pBT)cvReleaseBlobTracker(&param.pBT);
		if (param.pBD)cvReleaseBlobDetector(&param.pBD);
		if (param.pBTGen)cvReleaseBlobTrackGen(&param.pBTGen);
		if (param.pBTA)cvReleaseBlobTrackAnalysis(&param.pBTA);
		if (param.pFG)cvReleaseFGDetector(&param.pFG);
		if (pTracker)cvReleaseBlobTrackerAuto(&pTracker);

	}   /* Save state and release modules. */

	if (pCap)
		cvReleaseCapture(&pCap);
	cvDestroyWindow("TLD1");
	ReleaseCapture();
	return 1;
}
/*****************************************Auto******************************/


/******************************************face**********************************/
/*人脸跟踪*/
/********************************************************************************/
static CvHaarClassifierCascade* cascade = 0;
static CvMemStorage* storage = 0;
static int times = 0;
void detect_and_draw( IplImage* image );
const char* cascade_name ="haarcascade_frontalface_alt.xml";                //人脸检测要用到的分类器

void detect_and_draw( IplImage* img )
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
		for( int i = 0; i < (faces ? faces->total : 0); i++ )
		{
			CvRect* r = (CvRect*)cvGetSeqElem( faces, i );

			if (times == 1)
				return;
			if (r->height == 0)
			{
				detect_and_draw(img);
				cvReleaseImage( &gray );
				cvReleaseImage( &small_img );
				return ;
			}else
			{
				//一次赋值
				box.x = r->x+r->width*0.5;
				box.y = r->y+r->height*0.3;
				box.width = r->width;                 //数值为估计的
				box.height = r->height*1.2;
				times=1;
			}
			cvReleaseImage( &gray );
			cvReleaseImage( &small_img );
			return ;
		}
	//detect_and_draw(img);
	//cvShowImage( "result", img );

		cvReleaseImage( &gray );
		cvReleaseImage( &small_img );
		return ;
	}
	return;
}


int Face()
{
	  VideoCapture capture;
	  capture.open(0);
	  FileStorage fs;
	  fs.open("parameters.yml", FileStorage::READ);
	  tl = true;             // "tl" value determines the appearance of the rect with gezi
	  //Read options
	  //read_options(argc,argv,capture,fs);
	  //Init camera
	  if (!capture.isOpened())
	  {
		cout << "capture device failed to open!" << endl;
		return 2;
	  }
	  //Register mouse callback to draw the bounding box
	  cvNamedWindow("TLD2",CV_WINDOW_AUTOSIZE);
	  //cvSetMouseCallback( "TLD", mouseHandler, NULL );
	  TLD tld;
	  tld.read(fs.getFirstTopLevelNode());
		capture.set(CV_CAP_PROP_FRAME_WIDTH,640);
		capture.set(CV_CAP_PROP_FRAME_HEIGHT,400);

	  //TLD framework

	  //Read parameters file
 
	  Mat frame;
	  Mat last_gray;
	  Mat first;

	  //if (fromfile){
	  //    capture >> frame;
	  //    cvtColor(frame, last_gray, CV_RGB2GRAY);
	  //    frame.copyTo(first);
	  //}else{

	GETBOUNDINGBOX:  //}
  		Mat img;
		//Mat img3;
		(VideoCapture)capture >> img;
		IplImage image = img;
		IplImage* image2 = &image; 
		detect_and_draw(image2);
		//image = *image2;
		Mat img3(image2, false);
	  ///Initialization

	//  while(!gotBB)
	  //{
		if (!fromfile){
		  capture >> frame;
		}
		else
		  img3.copyTo(frame);
		img3.release();
		cvtColor(frame, last_gray, CV_RGB2GRAY);
		drawBox(frame,box);
		imshow("TLD2", frame);
		if (cvWaitKey(33) == 27)
		{
			cvDestroyWindow("TLD2");
			return 0;
		}
	//  }
	  if (min(box.width,box.height)<(int)fs.getFirstTopLevelNode()["min_win"]){
		  cout << "Bounding box too small, try again." << endl;
		  gotBB = false;
		  goto GETBOUNDINGBOX;
	  }
	  //Remove callback
	  cvSetMouseCallback( "TLD2", NULL, NULL );
	  printf("Initial Bounding Box = x:%d y:%d h:%d w:%d\n",box.x,box.y,box.width,box.height);
	  //Output file
	  FILE  *bb_file = fopen("bounding_boxes.txt","w");
	  //TLD initialization
	  tld.init(last_gray,box,bb_file);

	  ///Run-time
	  Mat current_gray;
	  BoundingBox pbox;
	  vector<Point2f> pts1;
	  vector<Point2f> pts2;
	  bool status=true;
	  int frames = 1;
	  int detections = 1;
	REPEAT:
	  while(capture.read(frame)){
		//get frame
		cvtColor(frame, current_gray, CV_RGB2GRAY);
		//Process Frame
		tld.processFrame(last_gray,current_gray,pts1,pts2,pbox,status,tl,bb_file);
		//Draw Points
		if (status){
		  drawPoints(frame,pts1);
		  drawPoints(frame,pts2,Scalar(0,255,0));
		  drawBox(frame,pbox);
		  detections++;
		}

		//Display
		imshow("TLD2", frame);
		//swap points and images
		swap(last_gray,current_gray);
		pts1.clear();
		pts2.clear();
		frames++;
		printf("Detection rate: %d/%d\n",detections,frames);
		if (cvWaitKey(33) == 27)
		{
			cvDestroyWindow("TLD2");
			break;
		}
	  }
	  if (rep){
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
	  cvDestroyWindow("TLD2");
	  return 2;
}

/************************************face**************************/


int main(int argc, char * argv[])
{
MODEL:
	cout << "***************************选择模式*****************************" << endl << endl;
	cout << "******************模式介绍：                            切换命令" << endl;
	cout << "********************模式一：手动多物体跟踪模式           hand   " << endl;
	cout << "********************模式二：自动单物体跟踪模式           auto   " << endl;
	cout << "********************模式三：人脸多物体跟踪模式           face   " << endl << endl;
	cout << "********************退出！                               quit   " << endl << endl;
	cout << "****************************************************************" << endl;
	string m_command;
CHOOSE:
	cout << endl << "请选择进入的模式：";
	cin >> m_command;

	if ( m_command == "quit" )
		return 0;
	else
		if ( m_command == "hand" )
			choose = '0';
		else 
			if ( m_command == "auto" )
			choose = '1';
			else 
				if ( m_command == "face" )
					choose = '2';
				else
					choose = 'o';

	int type;
	LPCWSTR str_type;

	switch (choose)
	{
	case '0':
		type = Hander(argc, argv);
		break;
	case '1':
		type = Auto(argc, argv);
		break;
	case '2':
		type = Face();
		break;
	default:
		int i = MessageBox( NULL, L"对不起！您输入的命令不存在！\r\n 是否要重新输入？", L"提示", MB_YESNO );
		switch (i)
		{
		case 6:
			goto CHOOSE;
		case 7:
			break;
		}
		break;
	}

	switch (type)
	{
	case 0:
		str_type = L"感谢您使用手动多物体跟踪模式！是否要切换其他模式？";
		break;
	case 1:
		str_type = L"感谢您使用自动单物体跟踪模式！是否要切换其他模式？";
		break;
	case 2:
		str_type = L"感谢您使用人脸多物体跟踪模式！是否要切换其他模式？";
		break;
	}

	if ( 6 == MessageBox( NULL, str_type, L"提示", MB_YESNO ) )
		goto MODEL;

	return 0;
 }
