#include "libretro.h"
#include "libretro-core.h"
#include "retroscreen.h"

#include <string>
#include <list>
#include <stdio.h>
#include <sstream>
#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "autoconf.h"
#include "options.h"
#include "gui.h"
#include "custom.h"
#include "uae.h"
#include "disk.h"

#include "memory.h"
#include "zfile.h"
#include "newcpu.h"
#include "savestate.h"
#include "audio.h"
#include "filesys.h"

//CORE VAR
#ifdef _WIN32
char slash = '\\';
#else
char slash = '/';
#endif
extern const char *retro_save_directory;
extern const char *retro_system_directory;
extern const char *retro_content_directory;
char RETRO_DIR[512];

char DISKA_NAME[512]="\0";
char DISKB_NAME[512]="\0";
char TAPE_NAME[512]="\0";
char CART_NAME[512]="\0";
extern char *dirSavestate;

//TIME
#ifdef __CELLOS_LV2__
#include "sys/sys_time.h"
#include "sys/timer.h"
#define usleep  sys_timer_usleep
#else
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#endif


typedef enum
{
	GUI_NONE = 0,
	GUI_LOAD ,
	GUI_SNAP ,
	GUI_MAIN ,

} GUI_ACTION;

int GUISTATE=GUI_NONE;

extern void Screen_SetFullUpdate(int scr);
extern bool Dialog_DoProperty(void);
extern void virtual_kdb(char *buffer,int vx,int vy);
extern int check_vkey2(int x,int y);

long frame=0;
unsigned long  Ktime=0 , LastFPSTime=0;

//VIDEO
#ifdef  RENDER16B
	uint16_t Retro_Screen[1280*1024];
#else
	unsigned int Retro_Screen[1280*1024];
#endif 

//SOUND
short signed int SNDBUF[1024*2];
int snd_sampler = 44100 / 50;

//PATH
char RPATH[512];

//EMU FLAGS  //apiso abilitazione pagina 3
int NPAGE=0, KCOL=1, BKGCOLOR=0;
int SHOWKEY=-1;

#if defined(ANDROID) || defined(__ANDROID__)
int MOUSE_EMULATED=1;
#else
int MOUSE_EMULATED=-1;
#endif
int MAXPAS=6,SHIFTON=-1,MOUSEMODE=-1,PAS=4;
int SND=1; //SOUND ON/OFF
static int firstps=0;
int pauseg=0; //enter_gui
int touch=-1; // gui mouse btn
//JOY
int al[2][2];//left analog1
int ar[2][2];//right analog1
//apiso modifico tipo mxjoy la metto uguale a quello in joystick.cpp 
//unsigned char MXjoy[2]; // joy
int MXjoy[2];
//int NUMjoy=1;
int NUMDRV=1;

int mouseemulated=0;

//apiso percorso del file di boot servirà come nome per i savestate
extern char bootdiskpth[MAX_DPATH];

//MOUSE
extern int pushi;  // gui mouse btn
int gmx,gmy; //gui mouse
int c64mouse_enable=0;
int mouse_wu=0,mouse_wd=0;
//KEYBOARD
char Key_Sate[512];
char Key_Sate2[512];
static char old_Key_Sate[512];

static int mbt[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//STATS GUI
int BOXDEC= 32+2;
int STAT_BASEY;

/*static*/ retro_input_state_t input_state_cb;
static retro_input_poll_t input_poll_cb;

extern void Keymap_KeyUp(int symkey);  // define in retrostubs.c
extern void Keymap_KeyDown(int symkey);  // define in retrostubs.c

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

int UnInitOSGLU(){
	return 0;
}

#ifndef NO_LIBCO
void goto_main_thd(){
co_switch(mainThread);
}
void goto_emu_thd(){
co_switch(emuThread);
}
#endif

long GetTicks(void)
{ // in MSec
#ifndef _ANDROID_

#ifdef __CELLOS_LV2__

   //#warning "GetTick PS3\n"

   unsigned long	ticks_micro;
   uint64_t secs;
   uint64_t nsecs;

   sys_time_get_current_time(&secs, &nsecs);
   ticks_micro =  secs * 1000000UL + (nsecs / 1000);

   return ticks_micro;///1000;
#else
   struct timeval tv;
   gettimeofday (&tv, NULL);
   return (tv.tv_sec*1000000 + tv.tv_usec);///1000;
#endif

#else

   struct timespec now;
   clock_gettime(CLOCK_MONOTONIC, &now);
   return (now.tv_sec*1000000 + now.tv_nsec/1000);///1000;
#endif

} 

#ifdef NO_LIBCO
extern void retro_run_gui(void);
#endif
int slowdown=0;
//NO SURE FIND BETTER WAY TO COME BACK IN MAIN THREAD IN HATARI GUI
void gui_poll_events(void)
{
   Ktime = GetTicks();

   if(Ktime - LastFPSTime >= 1000/50)
   {
	  slowdown=0;
      frame++; 
      LastFPSTime = Ktime;
#ifndef NO_LIBCO		
      co_switch(mainThread);
#else
	//FIXME nolibco Gui endless loop -> no retro_run() call
	  retro_run_gui();
#endif
   }
}

void retro_mouse(int a,int b) {}
void retro_mouse_but0(int a) {}
void retro_mouse_but1(int a) {}
void enter_options(void) {}

#if defined(ANDROID) || defined(__ANDROID__)
#define DEFAULT_PATH "/mnt/sdcard/"
#else

#ifdef PS3PORT
#define DEFAULT_PATH "/dev_hdd0/HOMEBREW/amstrad/"
#else
#define DEFAULT_PATH "/"
#endif

#endif

int STATUTON=-1;
#define RETRO_DEVICE_AMIGA_KEYBOARD RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_KEYBOARD, 0)
#define RETRO_DEVICE_AMIGA_JOYSTICK RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)

void enter_gui(void)
{
	switch (GUISTATE){
	
		case GUI_LOAD:
//			DlgFloppy_Main();
			break;
	
		case GUI_SNAP:
//			Dialog_SnapshotDlg();
			break;

		case GUI_MAIN:
//			Dialog_DoProperty();
			break;

		default:
			break;
	}

	pauseg=0;

}

void pause_select(void)
{
   if(pauseg==1 && firstps==0)
   {
      firstps=1;
      enter_gui();
      firstps=0;
   }
}

void texture_uninit(void)
{

}

void texture_init(void)
{
   memset(Retro_Screen, 0, sizeof(Retro_Screen));
   memset(old_Key_Sate ,0, sizeof(old_Key_Sate));

   gmx=(retrow/2)-1;
   gmy=(retroh/2)-1;
}

int bitstart=0;
int pushi=0; //mouse button
int keydown=0,keyup=0;
int KBMOD=-1;
int RSTOPON=-1;
int CTRLON=-1;

//extern unsigned short int bmp[400*300];
extern unsigned amiga_devices[ 2 ];

extern void vkbd_key(int key,int pressed);

#include "target.h"
#include "keyboard.h"
#include "keybuf.h"
#include "libretro-keymap.h"
extern int keycode2amiga(int prKeySym);


void retro_key_down(int key)
{
  int iAmigaKeyCode;
	iAmigaKeyCode =//keycode2amiga(key);
keyboard_translation[key];
//LOGI("kbdkey(%d)=%d pressed\n",key,iAmigaKeyCode);
unsigned char kkey = iAmigaKeyCode | 0x80;

  	if (iAmigaKeyCode >= 0)
  		if (!uae4all_keystate[iAmigaKeyCode])
  		{
  			uae4all_keystate[iAmigaKeyCode] = 1;
//record_key ((unsigned char)((kkey << 1) | (kkey >> 7)));
  			record_key(iAmigaKeyCode << 1);
  		}
  				
}

void retro_key_up(int key)
{
  int iAmigaKeyCode;
	iAmigaKeyCode =// keycode2amiga(key);
keyboard_translation[key];
//LOGI("kbdkey(%d)=%d released\n",key,iAmigaKeyCode);

unsigned char kkey = iAmigaKeyCode | 0x00;
  	if (iAmigaKeyCode >= 0)
	{
		uae4all_keystate[iAmigaKeyCode] = 0;
//record_key ((unsigned char)((kkey << 1) | (kkey >> 7)));
  		record_key((iAmigaKeyCode << 1) | 1);
  	}
}


void vkbd_key(int key,int pressed){

	//LOGI("key(%x)=%x shift:%d (%d)\n",key,pressed,SHIFTON,((key>>4)*8)+(key&7));


	int key2=key;//code2amiga(key);

	if(pressed){

		if(SHIFTON==1){
			uae4all_keystate[AK_LSH] = 1;
			record_key((AK_LSH << 1));
		}

		uae4all_keystate[key2] = 1;
		record_key(key2 << 1);
	}
	else {

		if(SHIFTON==1){
			uae4all_keystate[AK_LSH] = 0;
			record_key((AK_LSH << 1) | 1);
		}	
	
		uae4all_keystate[key2] = 0;
		record_key((key2 << 1) | 1);		
	}

}

void retro_virtualkb(void)
{
   // VKBD
   int i;
   //   RETRO        B    Y    SLT  STA  UP   DWN  LEFT RGT  A    X    L    R    L2   R2   L3   R3
   //   INDEX        0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15
   static int oldi=-1;
   static int vkx=0,vky=0;

   if(oldi!=-1)
   {
	  //retro_key_up(oldi);
	  vkbd_key(oldi,0);
      oldi=-1;
   }

   if(SHOWKEY==1)
   {
      static int vkflag[5]={0,0,0,0,0};		

      if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) && vkflag[0]==0 )
         vkflag[0]=1;
      else if (vkflag[0]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) )
      {
         vkflag[0]=0;
         vky -= 1; 
      }

      if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) && vkflag[1]==0 )
         vkflag[1]=1;
      else if (vkflag[1]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) )
      {
         vkflag[1]=0;
         vky += 1; 
      }

      if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) && vkflag[2]==0 )
         vkflag[2]=1;
      else if (vkflag[2]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) )
      {
         vkflag[2]=0;
         vkx -= 1;
      }

      if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) && vkflag[3]==0 )
         vkflag[3]=1;
      else if (vkflag[3]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) )
      {
         vkflag[3]=0;
         vkx += 1;
      }

      if(vkx<0)vkx=NPLGN-1;
      if(vkx>NPLGN-1)vkx=0;
      if(vky<0)vky=NLIGN-1;
      if(vky>NLIGN-1)vky=0;

      virtual_kdb(( char *)Retro_Screen,vkx,vky);
 
      i=8;
      if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i)  && vkflag[4]==0) 	
         vkflag[4]=1;
      else if( !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i)  && vkflag[4]==1)
      {
         vkflag[4]=0;
         i=check_vkey2(vkx,vky);

         if(i==-1){
            oldi=-1;
		 }
         if(i==-2)  //Gestione pagine
         {
			//apiso gestione 3 pagine 
			NPAGE=NPAGE+1;
			if 	(NPAGE>2)NPAGE=0;
            //NPAGE=-NPAGE;
			oldi=-1;
         }
         else if(i==-3) //KDB bgcolor
         {
            //KDB bgcolor
            KCOL=-KCOL;
            oldi=-1;
         }
         else if(i==-4)
         {
            //VKbd show/hide 			
            oldi=-1;
            Screen_SetFullUpdate(0);
            SHOWKEY=-SHOWKEY;
         }
         else if(i==-5)  //change disk
         {
			//FLIP DSK PORT 1-2
			//NUMDRV=-NUMDRV;
            
			bool plus=true;
			changedisk(plus);
			oldi=-1;
         }
         else if(i==-6)
         {
			//Exit
			retro_deinit();
			oldi=-1;
            exit(0);
         }
         else if(i==-7)
         {
			//SNA SAVE
			//snapshot_save (RPATH);
            oldi=-1;
         }
         else if(i==-8)
         {
			//PLAY TAPE
			//play_tape();
            oldi=-1;
         }

         else
         {

            if(i==AK_LSH /*i==-10*/) //SHIFT
            {
//			   if(SHIFTON == 1)retro_key_up(RETROK_RSHIFT);
//			   else retro_key_down(RETROK_LSHIFT);
               SHIFTON=-SHIFTON;

               oldi=-1;
            }
            else if(i==0x27/*i==-11*/) //CTRL
            {     
//         	   if(CTRLON == 1)retro_key_up(RETROK_LCTRL);
///			   else retro_key_down(RETROK_LCTRL);
               CTRLON=-CTRLON;

               oldi=-1;
            }
			else if(i==-12) //RSTOP
            {
//           	   if(RSTOPON == 1)retro_key_up(RETROK_ESCAPE);
//			   else retro_key_down(RETROK_ESCAPE);            
               RSTOPON=-RSTOPON;

               oldi=-1;
            }
			else if(i==-13) //GUI
            {     
			    //pauseg=1;
				//GUISTATE=GUI_MAIN;
				//SHOWKEY=-SHOWKEY;
				//Screen_SetFullUpdate(0);
					
				bool plus=false;
				changedisk(plus);
				oldi=-1;
            }
			else if(i==-14) //JOY PORT TOGGLE
            {    
 				//cur joy toggle
				//cur_port++;if(cur_port>2)cur_port=1;
               SHOWKEY=-SHOWKEY;
               oldi=-1;
            }
            
			else if(i>-42 and i<-29) //Save State
            {    
 			    int slot = -i -30 ;
				std::stringstream convert;   // stream used for the conversion
				convert << slot;      
				std::string strslot = convert.str();
				
				//std::string sfpath=changed_prefs.df[0];
				std::string sfpath=bootdiskpth;
				std::size_t ffinedisk = sfpath.find_last_of("/")+1;
				//wrong file name
				if (ffinedisk==std::string::npos)return ;
				std::string fname=sfpath.substr(ffinedisk,sfpath.length()-ffinedisk);
				//std::string fpath=sfpath.substr(0,ffinedisk);
				std::string sState = dirSavestate; 
				sState.append(fname);
				sState.append(".SAV");
				sState.append(strslot);
				//write_log("SAVE Slot: %d     Filename: %s \n",slot,sState.c_str());
				/*
				std::string fname=changed_prefs.df[0]; //+ ".SAV" + strslot;
				fname.append(".SAV");
				fname.append(strslot);
				write_log("SAVE Slot: %d     Filename: %s \n",slot,fname.c_str());
				*/
				if (fname.length() > 0)
				{
					//1 compressed
					savestate_initsave(sState.c_str(), 1, 1);
					save_state(sState.c_str(), "...");
//					savestate_state = STATE_DOSAVE; // Just to create the screenshot
//					delay_savestate_frame = 2;
//					gui_running = false;
				}
				//int lfname =fname.length();
				oldi=-1;
            }
			else if(i>-62 and i<-49) //Load State
            {    
 			    int slot = -i -50 ;
				std::stringstream convert;   // stream used for the conversion
				convert << slot;      
				std::string strslot = convert.str();
				//std::string sfpath=changed_prefs.df[0];
				std::string sfpath=bootdiskpth;
				std::size_t ffinedisk = sfpath.find_last_of("/")+1;
				//wrong file name
				if (ffinedisk==std::string::npos)return ;
				std::string fname=sfpath.substr(ffinedisk,sfpath.length()-ffinedisk);
				//std::string fpath=sfpath.substr(0,ffinedisk);
				std::string sState = dirSavestate; 
				sState.append(fname);
				sState.append(".SAV");
				sState.append(strslot);
				//write_log("LOAD Slot: %d     Filename: %s \n",slot,sState.c_str());
				//Verifico esistenza file
				if (fname.length() > 0)				{
					FILE * f = fopen(sState.c_str(), "rbe");
					if (f!=NULL)
					{
						fclose(f);
						savestate_initsave(sState.c_str(), 2, 0);
						savestate_state = STATE_DORESTORE;
						//gui_running = false;
						write_log("File trovato eseguito Restore \n");
					}
				}
				oldi=-1;
            }
			else
            {
               oldi=i;
			   //retro_key_down(oldi); 
			   vkbd_key(oldi,1);            
            }

         }
      }

	}


}

void Screen_SetFullUpdate(int scr)
{
   if(scr==0 ||scr>1)memset(Retro_Screen, 0, sizeof(Retro_Screen));
   //if(scr>0)memset(bmp,0,sizeof(bmp));
}

void Process_key()
{
	int i;

	for(i=0;i<320;i++)
        	Key_Sate[i]=input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0,i) ? 0x80: 0;
   
	if(memcmp( Key_Sate,old_Key_Sate , sizeof(Key_Sate) ) )
	 	for(i=0;i<320;i++)
			if(Key_Sate[i] && Key_Sate[i]!=old_Key_Sate[i]  )
        	{	
				if(i==RETROK_F12){
					//play_tape();
					continue;
				}
/*
				if(i==RETROK_RCTRL){
					//CTRLON=-CTRLON;
					printf("Modifier crtl pressed %d \n",CTRLON); 
					continue;
				}
				if(i==RETROK_RSHIFT){
					//SHITFON=-SHITFON;
					LOGI("Modifier shift pressed %d \n",SHIFTON); 
					continue;
				}
*/
				if(i==RETROK_LALT){
					//KBMOD=-KBMOD;
					LOGI("Modifier alt pressed %d \n",KBMOD); 
					continue;
				}
//printf("press: %d \n",i);
				retro_key_down(i);
	
        	}	
        	else if ( !Key_Sate[i] && Key_Sate[i]!=old_Key_Sate[i]  )
        	{
				if(i==RETROK_F12){
      				//kbd_buf_feed("|tape\nrun\"\n^");
					continue;
				}
/*
				if(i==RETROK_RCTRL){
					CTRLON=-CTRLON;
					printf("Modifier crtl released %d \n",CTRLON); 
					continue;
				}
				if(i==RETROK_RSHIFT){
					SHIFTON=-SHIFTON;
					printf("Modifier shift released %d \n",SHIFTON); 
					continue;
				}
*/
				if(i==RETROK_LALT){
					KBMOD=-KBMOD;
					LOGI("Modifier alt released %d \n",KBMOD); 
					continue;
				}
//printf("release: %d \n",i);
				retro_key_up(i);
	
        	}	

	memcpy(old_Key_Sate,Key_Sate , sizeof(Key_Sate) );

}
/*
void Print_Statut(void)
{
   DrawFBoxBmp(bmp,0,CROP_HEIGHT,CROP_WIDTH,STAT_YSZ,RGB565(0,0,0));

   Draw_text(bmp,STAT_DECX+40 ,STAT_BASEY,0xffff,0x8080,1,2,40,(SND>0?"SND":""));
   Draw_text(bmp,STAT_DECX+80 ,STAT_BASEY,0xffff,0x8080,1,2,40,"F:%d",dwFPS);
   Draw_text(bmp,STAT_DECX+120,STAT_BASEY,0xffff,0x8080,1,2,40,"DSK%c",NUMDRV>0?'A':'B');
   if(ZOOM>-1)
      Draw_text(bmp,(384-Mres[ZOOM].x)/2,(272-Mres[ZOOM].y)/2,0xffff,0x8080,1,2,40,"x:%3d y:%3d",Mres[ZOOM].x,Mres[ZOOM].y);
}
*/


/*
In joy mode
L3  GUI LOAD
R3  GUI SNAP 
L2  STATUS ON/OFF
R2  AUTOLOAD TAPE
L   CAT
R   RESET
SEL MOUSE/JOY IN GUI
STR ENTER/RETURN
A   FIRE1/VKBD KEY
B   RUN
X   FIRE2
Y   VKBD ON/OFF
In Keayboard mode
F8 LOAD DSK/TAPE
F9 MEM SNAPSHOT LOAD/SAVE
F10 MAIN GUI
F12 PLAY TAPE
*/
extern int lastmx, lastmy, newmousecounters;
extern int buttonstate[3];

int Retro_PollEvent()
{
    //   RETRO        B    Y    SLT  STA  UP   DWN  LEFT RGT  A    X    L    R    L2   R2   L3   R3
    //   INDEX        0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15

   int SAVPAS=PAS;	
   int i;
   static int vbt[16]={0x0,0x0,0x0,0x0,0x01,0x02,0x04,0x08,0x10,0x20,0x0,0x0,0x0,0x0,0x0,0x0};
   MXjoy[0]=0;
   MXjoy[1]=0;
   input_poll_cb();

   int mouse_l;
   int mouse_r;
   int16_t rmouse_x,rmouse_y;
   rmouse_x=rmouse_y=0;

   if(SHOWKEY==-1 && pauseg==0)Process_key();

   if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F8)){ 
		GUISTATE=GUI_LOAD;
		pauseg=1;
   }
   if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F9)){ 
		GUISTATE=GUI_SNAP;
		pauseg=1;
   }
   if (input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_F10)){ 
		GUISTATE=GUI_MAIN;
		pauseg=1;
   }

 
	if(pauseg==0){ // if emulation running

		  //Joy mode
		  //apiso tento di abilitare joy1 apparentemente adesso viene impostato con i valori dello 0
	  if(SHOWKEY==-1)
	  {
		for(i=4;i<10;i++)
		{
			 if( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i))
			 {
					MXjoy[0] |= vbt[i]; // Joy press
					//MXjoy[1] |= vbt[i]; // Joy press
			
			 }
			 //apiso leggo imput joy1 e setto mxjoy1 
			 //penso sia stato fatto cosi' per l'utilizzo del mouse sempre con il joy 0 
			 if( input_state_cb(1, RETRO_DEVICE_JOYPAD, 0, i))
			 {
					//MXjoy[0] |= vbt[i]; // Joy press
					MXjoy[1] |= vbt[i]; // Joy press
					//qui entra correttamente
					//write_log("Entrato Joy1 I:%d   state:%d \n" ,i,vbt[i]);
			 }
		  }
		}
		  //if(SHOWKEY==-1)retro_joy0_test(MXjoy[0]);


	if(amiga_devices[0]==RETRO_DEVICE_AMIGA_JOYSTICK){
	   //shortcut for joy mode only

	   i=1;//show vkbd toggle
	   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
		  mbt[i]=1;
	   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
	   {
		  mbt[i]=0;
		  SHOWKEY=-SHOWKEY;
	   }

	   i=3;//type ENTER
	   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
		  mbt[i]=1;
	   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )
	   {
		  mbt[i]=0;
		  //kbd_buf_feed("\n");
	   }

	/*
	   i=10;//type DEL / ZOOM
	   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
		  mbt[i]=1;
	   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
		  mbt[i]=0;
		  ZOOM++;if(ZOOM>4)ZOOM=-1;

	   }
	*/

	   i=0;//type RUN"
	   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
		  mbt[i]=1;
	   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
		  mbt[i]=0;
		  //kbd_buf_feed("RUN\"");
	   }

	   i=10;//Type CAT\n 
	   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
		  mbt[i]=1;
	   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
		  mbt[i]=0;
		  //kbd_buf_feed("CAT\n");
		  //Screen_SetFullUpdate();
	   }

	   i=12;//show/hide statut
	   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
		  mbt[i]=1;
	   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
		  mbt[i]=0;
		  STATUTON=-STATUTON;
		 // Screen_SetFullUpdate();
	   }

	   i=13;//auto load tape
	   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
		  mbt[i]=1;
	   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
		  mbt[i]=0;
		  //kbd_buf_feed("|tape\nrun\"\n^");
	   }

	   i=11;//reset
	   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
		  mbt[i]=1;
	   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
		  mbt[i]=0;
		 // emu_reset();		
	   }
	/*
	   i=2;//mouse/joy toggle
	   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
		  mbt[i]=1;
	   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
		  mbt[i]=0;
		  MOUSE_EMULATED=-MOUSE_EMULATED;
	   }
	*/
	   // L3 -> gui load
	   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, 14)){ 
			GUISTATE=GUI_LOAD;
			pauseg=1;
	   }
	   // R3 -> gui snapshot
	   if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, 15)){ 
			GUISTATE=GUI_SNAP;
			pauseg=1;
	   }

	}//if amiga_devices=joy


	}// if pauseg=0
	else{
	   // if in gui
	/*
	   i=2;//mouse/joy toggle
	   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
		  mbt[i]=1;
	   else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
		  mbt[i]=0;
		  MOUSE_EMULATED=-MOUSE_EMULATED;
	   }
	*/
	}
	if(SHOWKEY==-1)
	  {
		
			i=2;//mouse/joy toggle
			if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
			  mbt[i]=1;
			else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
			  mbt[i]=0;
			  MOUSE_EMULATED=-MOUSE_EMULATED;
			}

			if(MOUSE_EMULATED==1){
			mouseemulated=1;
			//printf("-----------------mouse %d \n",pauseg);
			  if(slowdown>0 && pauseg!=0 )return 1;
			//printf("-----------------mouse %d \n",pauseg);
			  if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))rmouse_x += PAS;
			  if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))rmouse_x -= PAS;
			  if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))rmouse_y += PAS;
			  if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))rmouse_y -= PAS;
			  mouse_l=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
			  mouse_r=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);

			  PAS=SAVPAS;

			  slowdown=1;
			}
			else {
			   //apiso se non ho premuto il tasto di emulazione mouse metto a 0 pandora_custom_dpad
			   //questo dovrebbe consentire nella imputdevice di scegliere se restituire i valori 
			   //del joy o quelli del mouse 
			   mouseemulated=0;

			//printf("-----------------%d \n",pauseg);
			  mouse_wu = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP);
			  mouse_wd = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN);
			//if(mouse_wu || mouse_wd)printf("-----------------MOUSE UP:%d DOWN:%d\n",mouse_wu, mouse_wd);
			  rmouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
			  rmouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
			  mouse_l    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
			  mouse_r    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
			}
			//write_log("core-mapper mouseemulated:%d \n",mouseemulated);
			static int mmbL=0,mmbR=0;

			//apiso
			//write_log("Stato Button mmbL: %d   mouse_l: %d",mmbL,mouse_l);

			if(mmbL==0 && mouse_l){

			  mmbL=1;		
			  pushi=1;
			  touch=1;

			}
			else if(mmbL==1 && !mouse_l) {

			  mmbL=0;
			  pushi=0;
			  touch=-1;
			}

			//apiso
			//write_log("Stato Button mmbR: %d   mouse_r: %d",mmbR,mouse_r);
			if(mmbR==0 && mouse_r){
			  mmbR=1;		
			  //apiso abilitazione pulsante destro mouse
			  pushi=1;
			  touch=1;
			}
			else if(mmbR==1 && !mouse_r) {
			  mmbR=0;
			  //apiso abilitazione pulsante destro mouse
			  pushi=0;
			  touch=-1;
			}

			//if(pauseg==0 && c64mouse_enable){
			/*
			mouse_move((int)mouse_x, (int)mouse_y);
			mouse_button(0,mmbL);
			mouse_button(1,mmbR);
			*/
			//}

			gmx+=rmouse_x;
			gmy+=rmouse_y;
			if(gmx<0)gmx=0;
			if(gmx>retrow-1)gmx=retrow-1;
			if(gmy<0)gmy=0;
			if(gmy>retroh-1)gmy=retroh-1;

			lastmx +=rmouse_x;
			lastmy +=rmouse_y;
			newmousecounters=1;
			if(pauseg==0) buttonstate[0] = mmbL;

			//apiso il bottone right è il 2 nella libreria SDL
			if(pauseg==0) buttonstate[2] = mmbR;

			// if(SHOWKEY && pauseg==0)retro_virtualkb();
		}
   return 1;

}


int update_input_gui(void)
{
   int ret=0;	
   static int dskflag[7]={0,0,0,0,0,0,0};// UP -1 DW 1 A 2 B 3 LFT -10 RGT 10 X 4	

   input_poll_cb();	

   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) && dskflag[0]==0 )
   {
      dskflag[0]=1;
      ret= -1; 
   }
   else
      dskflag[0]=0;

   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) && dskflag[1]==0 )
   {
      dskflag[1]=1;
      ret= 1;
   } 
   else
      dskflag[1]=0;

   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) && dskflag[4]==0 )
      dskflag[4]=1;
   else if (dskflag[4]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) )
   {
      dskflag[4]=0;
      ret= -10; 
   }

   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) && dskflag[5]==0 )
      dskflag[5]=1;
   else if (dskflag[5]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) )
   {
      dskflag[5]=0;
      ret= 10; 
   }

   if ( ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) || \
            input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LCTRL) ) && dskflag[2]==0 )
      dskflag[2]=1;

   else if (dskflag[2]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A)\
         && !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LCTRL) )
   {
      dskflag[2]=0;
      ret= 2;
   }

   if ( ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) || \
            input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LALT) ) && dskflag[3]==0 )
      dskflag[3]=1;
   else if (dskflag[3]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) &&\
         !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LALT) )
   {
      dskflag[3]=0;
      ret= 3;
   }

   if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X) && dskflag[6]==0 )
      dskflag[6]=1;
   else if (dskflag[6]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X) )
   {
      dskflag[6]=0;
      ret= 4; 
   }

   return ret;
}

