#include "sysconfig.h"
#include "sysdeps.h"
#include <string>
#include <list>
#include <stdio.h>
#include <sstream>
#include <ctype.h>

#include "options.h"
#include "uae.h"
#include "audio.h"
#include "autoconf.h"
#include "custom.h"
#include "inputdevice.h"
#include "savestate.h"
#include "memory.h"
#include "gui.h"
#include "newcpu.h"
#include "zfile.h"
#include "filesys.h"
#include "fsdb.h"
#include "disk.h"
#if defined(__LIBRETRO__)
#include "sd-retro/sound.h"
#else
#include "sd-pandora/sound.h"
#endif


#include "libretro.h"

#include "libretro-core.h"


#ifndef NO_LIBCO
cothread_t mainThread;
cothread_t emuThread;
#else
//extern void quit_vice_emu();
#endif

int CROP_WIDTH;
int CROP_HEIGHT;
int VIRTUAL_WIDTH ;
int retrow=1024; 
int retroh=1024;
unsigned short int bmp[400*300];

#define RETRO_DEVICE_AMIGA_KEYBOARD RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_KEYBOARD, 0)
#define RETRO_DEVICE_AMIGA_JOYSTICK RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)

unsigned amiga_devices[ 2 ];

int autorun=0;

int RETROJOY=0,RETROPT0=0,RETROSTATUS=0,RETRODRVTYPE=0;
int retrojoy_init=0,retro_ui_finalized=0;

extern int SHIFTON,pauseg,SND ,snd_sampler;
extern short signed int SNDBUF[1024*2];
extern char RPATH[512];
extern char RETRO_DIR[512];
int cap32_statusbar=0;
int arnold_model=2;

extern int SHOWKEY;

#include "cmdline.cpp"

extern void update_input(void);
extern void texture_init(void);
extern void texture_uninit(void);
extern void Emu_init();
extern void Emu_uninit();
extern void input_gui(void);
extern int UnInitOSGLU();
extern void retro_virtualkb(void);

const char *retro_save_directory;
const char *retro_system_directory;
const char *retro_content_directory;
char retro_system_data_directory[512];;

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;

//indica lo slot da usare per l'autoload, se 0 disabilitato
int autoloadslot = 0;
char *dirSavestate="/storage/system/uae4arm/savestates/";


//apiso
#define EMULATOR_DEF_WIDTH 320
#define EMULATOR_DEF_HEIGHT 240

// Amiga default models

#define A500 "\
cpu_type=68000\n\
chipmem_size=2\n\
bogomem_size=7\n\
chipset=ocs\n"

#define A600 "\
cpu_type=68000\n\
chipmem_size=2\n\
fastmem_size=4\n\
chipset=ecs\n"

#define A1200 "\
cpu_type=68ec020\n\
chipmem_size=4\n\
fastmem_size=8\n\
chipset=aga\n"

// Amiga default kickstarts

#define A500_ROM 	"/storage/system/kick34005.A500"
#define A600_ROM 	"/storage/system/kick40063.A600"
#define A1200_ROM 	"/storage/system/kick40068.A1200"

#define CFGFILE     "/storage/system/uae4arm/conf/uaeconfig.uae"

static char uae_machine[256];
static char uae_kickstart[16];
static char uae_config[1024];

int ledtype; //true false into the file
int defaultw = EMULATOR_DEF_WIDTH;
int defaulth = EMULATOR_DEF_HEIGHT;

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

  static const struct retro_controller_description p1_controllers[] = {
    { "AMIGA Joystick", RETRO_DEVICE_AMIGA_JOYSTICK },
    { "AMIGA Keyboard", RETRO_DEVICE_AMIGA_KEYBOARD },
  };
  static const struct retro_controller_description p2_controllers[] = {
    { "AMIGA Joystick", RETRO_DEVICE_AMIGA_JOYSTICK },
    { "AMIGA Keyboard", RETRO_DEVICE_AMIGA_KEYBOARD },
  };


  static const struct retro_controller_info ports[] = {
    { p1_controllers, 2  }, // port 1
    { p2_controllers, 2  }, // port 2
    { NULL, 0 }
  };

  cb( RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports );

      struct retro_variable variables[] = {
     { "uae4arm_autorun","Autorun; disabled|enabled" , },
	 { "uae4arm_model", "Model; A600|A1200|A500", },
     { "uae4arm_chipmemorysize", "Chip Memory Size; Default|512K|1M|2M|4M", },
	 {
         "uae4arm_resolution",
         "Internal resolution; 320x240|640x256|640x400|640x480|768x270|800x600|1024x768|1280x960",
      },
	 { "uae4arm_autoloadstate","AutoLoad State; Disabled|1|2|3|4|5|6|7|8|9|10|11|12", },
     //{ "puae_analog","Use Analog; OFF|ON", },
     { "uae4arm_leds","Leds on screen; True|False", },
     { "uae4arm_cpu_speed", "CPU speed; real|max", },
     { "uae4arm_cpu_compatible", "CPU compatible; true|false", },
     { "uae4arm_collision", "Collision level; none|sprites|playfields|full", },
     //{ "uae4arm_sound_mode", "Sound mode; SND_STEREO|SND_MONO|SND_4CH_CLONEDSTEREO|SND_4CH|SND_6CH_CLONEDSTEREO|SND_6CH|SND_NONE", },
	 //{ "uae4arm_sound_output", "Sound output; normal|exact|interrupts|none", },
     //{ "uae4arm_sound_frequency", "Sound frequency; 44100|22050|11025", },
     //{ "uae4rm_sound_interpol", "Sound interpolation; none|rh|crux|sync", },
     { "uae4arm_floppy_speed", "Floppy speed; 100|200|300|400|500|600|700|800", },
     { "uae4arm_immediate_blits", "Immediate blit; false|true", },
     { "uae4arm_framerate", "Frameskipping; 0|1", },
     { "uae4arm_ntsc", "NTSC Mode; false|true", },
     { "uae4arm_refreshrate", "Chipset Refresh Rate; 50|60", },
     //{ "puae_gfx_linemode", "Line mode; double|none", }, // Removed scanline, we have shaders for this
     { "uae4arm_gfx_correct_aspect", "Correct aspect ratio; true|false", },
     //{ "puae_gfx_center_vertical", "Vertical centering; simple|smart|none", },
     //{ "puae_gfx_center_horizontal", "Horizontal centering; simple|smart|none", },
	 
	 { NULL, NULL },
   };
   cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

//apiso provo a settare direttamente le variabili funzionerà?
static void update_variables(void)
{
   //apiso carico default values
   default_prefs (&changed_prefs, 0);
   default_prefs (&currprefs, 0);
   uae_machine[0] = '\0';
   uae_config[0] = '\0';
   struct retro_variable var = {0};

   var.key = "uae4arm_autorun";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
     if (strcmp(var.value, "enabled") == 0)
			 autorun = 1;
   }
   
   var.key = "uae4arm_resolution";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      char *pch;
      char str[100];
      snprintf(str, sizeof(str), var.value);

      pch = strtok(str, "x");
      if (pch)
         retrow = strtoul(pch, NULL, 0);
      pch = strtok(NULL, "x");
      if (pch)
         retroh = strtoul(pch, NULL, 0);

	//FIXME remove force res
	//retrow=WINDOW_WIDTH;
	//retroh=WINDOW_HEIGHT;

	//retrow=320;
	//retroh=240;

      LOGI("[libretro-vice]: Got size: %u x %u.\n", retrow, retroh);

	  //apiso set preference 
	  currprefs.gfx_size.width = retrow;
	  currprefs.gfx_size.height = retroh;
	  currprefs.gfx_size_win.width = retrow;
      currprefs.gfx_size_win.height = retroh;
      currprefs.gfx_size_fs.width = retrow;
      currprefs.gfx_size_fs.height = retroh;
  
      CROP_WIDTH =retrow;
      CROP_HEIGHT= (retroh-80);
      VIRTUAL_WIDTH = retrow;
      texture_init();
      //reset_screen();
   }


  /* var.key = "puae_analog";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "OFF") == 0)
        opt_analog = false;
      if (strcmp(var.value, "ON") == 0)
        opt_analog = true;
      ledtype = 1;
   }*/
   var.key = "uae4arm_autoloadstate";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "Disabled") == 0)
	  {
		  autoloadslot=0;
	  }
      else
	  {
		//Implementazione dell'autoload dello state sullo slot 12, serve   
        autoloadslot = atoi(var.value) ;
		  
      }
   }
   
   var.key = "uae4arm_leds";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "True") == 0)
      {
        currprefs.leds_on_screen = 1;        
      }
      if (strcmp(var.value, "False") == 0)
      {
        currprefs.leds_on_screen = 0;  
      }
   }

   var.key = "uae4arm_collision";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "none") == 0)
      {
        currprefs.collision_level = 0;        
      }
      if (strcmp(var.value, "sprites") == 0)
      {
        currprefs.collision_level = 1;  
      }
	     if (strcmp(var.value, "playfields") == 0)
      {
        currprefs.collision_level = 2;  
      }
	     if (strcmp(var.value, "full") == 0)
      {
        currprefs.collision_level = 3;  
      }
   }
   
   var.key = "uae4arm_model";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		if (strcmp(var.value, "A500") == 0)
		{
			//strcat(uae_machine, A500);
			//strcpy(uae_kickstart, A500_ROM);
			//currprefs.cpu_type="68000";
			
			currprefs.cpu_model = 68000;
			//currprefs.m68k_speed = 0;
			//currprefs.cpu_compatible = 0;
			currprefs.chipmem_size = 2 * 0x80000;
			currprefs.address_space_24 = 1;
			currprefs.chipset_mask = CSMASK_ECS_DENISE;
			strcpy(currprefs.romfile, A500_ROM);
		}
		if (strcmp(var.value, "A600") == 0)
		{
			//strcat(uae_machine, A600);
			//strcpy(uae_kickstart, A600_ROM);
			currprefs.cpu_model = 68000;
			currprefs.chipmem_size = 2 * 0x80000;
			//currprefs.m68k_speed = 0;
			//currprefs.cpu_compatible = 0;
			currprefs.address_space_24 = 1;
			currprefs.chipset_mask = CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS;
			strcpy(currprefs.romfile, A600_ROM);
		}
		if (strcmp(var.value, "A1200") == 0)
		{
			//strcat(uae_machine, A1200);
			//strcpy(uae_kickstart, A1200_ROM);
			//currprefs.cpu_type="68ec020";
			currprefs.cpu_model = 68020;
			//currprefs.m68k_speed = 0;
			//currprefs.cpu_compatible = 0;
			currprefs.address_space_24 = 1;
			currprefs.chipset_mask = CSMASK_AGA | CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS;
			currprefs.chipmem_size = 4 * 0x80000;
		    strcpy(currprefs.romfile, A1200_ROM);
		}
   }

   var.key = "uae4arm_chipmemorysize";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      
	  //Default|512K|1M|2M|4M|8M
	  if (strcmp(var.value, "Default") == 0)
      {
		//se default tengo quello di default per il sistema  settata sopra 
        //currprefs.leds_on_screen = 1;        
      }
      if (strcmp(var.value, "512K") == 0)
      {
       currprefs.chipmem_size = 1 * 0x80000;
      }
	  if (strcmp(var.value, "1M") == 0)
      {
       currprefs.chipmem_size = 2 * 0x80000; 
      }
      if (strcmp(var.value, "2M") == 0)
      {
       currprefs.chipmem_size = 4 * 0x80000;
      }
	   if (strcmp(var.value, "4M") == 0)
      {
       currprefs.chipmem_size = 8 * 0x80000;
      }
	  
   }
   
   var.key = "uae4arm_cpu_speed";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	  	//strcat(uae_config, "cpu_speed=");
		//strcat(uae_config, var.value);
		//strcat(uae_config, "\n");
	   if (strcmp(var.value, "max") == 0)
		{
			currprefs.m68k_speed = -1;
		}
		else
		{
			currprefs.m68k_speed = 0;
		}
	  
   }

   var.key = "uae4arm_cpu_compatible";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		//strcat(uae_config, "cpu_compatible=");
		if (strcmp(var.value, "true") == 0)
		{
			currprefs.cpu_compatible = 1;
		}
		else
		{
			currprefs.cpu_compatible = 0;
		}
		//strcat(uae_config, var.value);
		//strcat(uae_config, "\n");
   }
/*
   var.key = "uae4arm_sound_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "SND_MONO") == 0)
      {
        currprefs.sound_stereo = 0;        
      }
      if (strcmp(var.value, "SND_STEREO") == 0)
      {
        currprefs.sound_stereo = 1;  
      }
	  if (strcmp(var.value, "SND_4CH_CLONEDSTEREO") == 2)
      {
        currprefs.sound_stereo = 2;  
      }
      if (strcmp(var.value, "SND_4CH") == 0)
      {
        currprefs.sound_stereo = 3;  
      }
      if (strcmp(var.value, "SND_6CH_CLONEDSTEREO") == 0)
      {
        currprefs.sound_stereo = 4;  
      }
      if (strcmp(var.value, "SND_6CH") == 0)
      {
        currprefs.sound_stereo = 5;  
      }
      if (strcmp(var.value, "SND_NONE") == 0)
      {
        currprefs.sound_stereo = 6;  
      }
   }
   
   var.key = "uae4arm_sound_output";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "normal") == 0)
      {
        currprefs.produce_sound = 0;        
      }
      if (strcmp(var.value, "exact") == 0)
      {
        currprefs.produce_sound = 1;  
      }
	     if (strcmp(var.value, "interrupts") == 0)
      {
        currprefs.produce_sound = 2;  
      }
	     if (strcmp(var.value, "none") == 0)
      {
        currprefs.produce_sound = 3;  
      }
   
   }

   var.key = "uae4arm_sound_frequency";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		currprefs.sound_freq=atoi(var.value);
   }

  
   var.key = "uae4rm_sound_interpol";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	  if (strcmp(var.value, "none") == 0)
      {
        currprefs.sound_interpol = 0;        
      }
      if (strcmp(var.value, "anti") == 0)
      {
        currprefs.sound_interpol = 1;  
      }
	  if (strcmp(var.value, "sinc") == 0)
      {
        currprefs.sound_interpol = 2;  
      }
	  if (strcmp(var.value,  "rh") == 0)
      {
        currprefs.sound_interpol = 3;  
      }
	  if (strcmp(var.value,  "crux") == 0)
      {
        currprefs.sound_interpol = 4;  
      }
   }
*/
   var.key = "uae4arm_floppy_speed";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	    currprefs.floppy_speed=atoi(var.value);
   }

   
   var.key = "uae4arm_immediate_blits";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		if (strcmp(var.value, "true") == 0)
		{
			currprefs.immediate_blits = 1;
		}
		else
		{
			currprefs.immediate_blits = 0;
		}
   }
   var.key = "uae4arm_framerate";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	    currprefs.gfx_framerate=atoi(var.value);
   }

   var.key = "uae4arm_ntsc";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		if (strcmp(var.value, "true") == 0)
		{
			currprefs.ntscmode = 1;
		}
		else
		{
			currprefs.ntscmode = 0;
		}
   }

   var.key = "uae4arm_refreshrate";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	    currprefs.chipset_refreshrate=atoi(var.value);
   }

   /*var.key = "puae_gfx_linemode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		strcat(uae_config, "gfx_linemode=");
		strcat(uae_config, var.value);
		strcat(uae_config, "\n");
   }*/

   var.key = "uae4arm_gfx_correct_aspect";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		//strcat(uae_config, "gfx_correct_aspect=");
		//strcat(uae_config, var.value);
		//strcat(uae_config, "\n");
		if (strcmp(var.value, "true") == 0)
		{
			currprefs.gfx_correct_aspect = 1;
		}
		else
		{
			currprefs.gfx_correct_aspect = 0;
		}
   }

  /* var.key = "puae_gfx_center_vertical";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		strcat(uae_config, "gfx_center_vertical=");
		strcat(uae_config, var.value);
		strcat(uae_config, "\n");
   }

   var.key = "puae_gfx_center_horizontal";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		strcat(uae_config, "gfx_center_horizontal=");
		strcat(uae_config, var.value);
		strcat(uae_config, "\n");
   }*/
}


static void retro_wrap_emulator()
{    

   pre_main(RPATH);

#ifndef NO_LIBCO
LOGI("EXIT EMU THD\n");
   pauseg=-1;

   //environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, 0); 

   // Were done here
   co_switch(mainThread);

   // Dead emulator, but libco says not to return
   while(true)
   {
      LOGI("Running a dead emulator.");
      co_switch(mainThread);
   }
#endif
}

void Emu_init(){

#ifdef RETRO_AND
   MOUSEMODE=1;
#endif

 //  update_variables();

   memset(Key_Sate,0,512);
   memset(Key_Sate2,0,512);

#ifndef NO_LIBCO
   if(!emuThread && !mainThread)
   {
      mainThread = co_active();
      emuThread = co_create(8*65536*sizeof(void*), retro_wrap_emulator);
   }
#else
	retro_wrap_emulator();
#endif
   update_variables();
}

void Emu_uninit(){
#ifdef NO_LIBCO
	//quit_cap32_emu();
#endif
   texture_uninit();
}

void retro_shutdown_core(void)
{
   LOGI("SHUTDOWN\n");
//main_exit();
#ifdef NO_LIBCO
	//quit_vice_emu();
#endif
   texture_uninit();
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

void retro_reset(void){
//      machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
	//emu_reset();
}

void retro_init(void)
{    	
   const char *system_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
   {
      // if defined, use the system directory			
      retro_system_directory=system_dir;		
   }		   

   const char *content_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &content_dir) && content_dir)
   {
      // if defined, use the system directory			
      retro_content_directory=content_dir;		
   }			

   const char *save_dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir)
   {
      // If save directory is defined use it, otherwise use system directory
      retro_save_directory = *save_dir ? save_dir : retro_system_directory;      
   }
   else
   {
      // make retro_save_directory the same in case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY is not implemented by the frontend
      retro_save_directory=retro_system_directory;
   }

   if(retro_system_directory==NULL)sprintf(RETRO_DIR, "%s\0",".");
   else sprintf(RETRO_DIR, "%s\0", retro_system_directory);

   sprintf(retro_system_data_directory, "%s/data\0",RETRO_DIR);

   LOGI("Retro SYSTEM_DIRECTORY %s\n",retro_system_directory);
   LOGI("Retro SAVE_DIRECTORY %s\n",retro_save_directory);
   LOGI("Retro CONTENT_DIRECTORY %s\n",retro_content_directory);

#ifndef RENDER16B
    	enum retro_pixel_format fmt =RETRO_PIXEL_FORMAT_XRGB8888;
#else
    	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
#endif
   
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      fprintf(stderr, "PIXEL FORMAT is not supported.\n");
LOGI("PIXEL FORMAT is not supported.\n");
      exit(0);
   }

	struct retro_input_descriptor inputDescriptors[] = {
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "A" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "B" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "X" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Y" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "R" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "L" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "R2" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "L2" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "R3" },
		{ 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "L3" }
	};
	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &inputDescriptors);
#ifndef NO_LIBCO
   Emu_init();
#endif
   texture_init();

}

extern void main_exit();
void retro_deinit(void)
{	 
   Emu_uninit(); 

   UnInitOSGLU();	

#ifndef NO_LIBCO
   co_switch(emuThread);
LOGI("exit emu\n");
  // main_exit();
   co_switch(mainThread);
LOGI("exit main\n");
   if(emuThread)
   {	 
      co_delete(emuThread);
      emuThread = 0;
   }
#endif

   LOGI("Retro DeInit\n");
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}


void retro_set_controller_port_device( unsigned port, unsigned device )
{
  if ( port < 2 )
  {
    amiga_devices[ port ] = device;

LOGI(" (%d)=%d \n",port,device);
  }
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "uae4arm";
   info->library_version  = "0.1";
   info->valid_extensions = "adf|dms|zip|ipf|adz";
   info->need_fullpath    = true;
   info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
//FIXME handle vice PAL/NTSC
   struct retro_game_geometry geom = { retrow, retroh, 1280, 1024,4.0 / 3.0 };
   struct retro_system_timing timing = { 50.0, 44100.0 };

   info->geometry = geom;
   info->timing   = timing;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_audio_cb( short l, short r)
{
	audio_cb(l,r);
}

void retro_audiocb(signed short int *sound_buffer,int sndbufsize){
 	int x;
    if(pauseg==0)for(x=0;x<sndbufsize;x+=2)audio_cb(sound_buffer[x],sound_buffer[x+1]);	
}


#ifdef NO_LIBCO
//FIXME nolibco Gui endless loop -> no retro_run() call
void retro_run_gui(void)
{
   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables();

   Retro_PollEvent();

   video_cb(Retro_Screen,retrow,retroh,retrow<<PIXEL_BYTES);
}
#endif
extern void testsnd(int len);

void retro_run(void)
{
   int x;

   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_variables();

   if(pauseg==0)
   {

//testsnd(882*2*2);
//for(x=0;x<882*2;x+=2)audio_cb(SNDBUF[x],SNDBUF[x+2]);
//Fix for audio with SNDBUF[x+2] audio elaborate only one channel 
for(x=0;x<882*2;x+=2)audio_cb(SNDBUF[x],SNDBUF[x+1]);


		if(SHOWKEY )retro_virtualkb();
   }

   video_cb(Retro_Screen,retrow,retroh,retrow<<PIXEL_BYTES);

#ifndef NO_LIBCO   
   co_switch(emuThread);
#endif

}

unsigned int lastdown,lastup,lastchar;
static void keyboard_cb(bool down, unsigned keycode,
      uint32_t character, uint16_t mod)
{
/*
  printf( "Down: %s, Code: %d, Char: %u, Mod: %u.\n",
         down ? "yes" : "no", keycode, character, mod);
*/
/*
if(down)lastdown=keycode;
else lastup=keycode;
lastchar=character;
*/
}


bool retro_load_game(const struct retro_game_info *info)
{
   const char *full_path;

   (void)info;


   struct retro_keyboard_callback cb = { keyboard_cb };
   environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);

   full_path = info->path;

   strcpy(RPATH,full_path);

   update_variables();

#ifdef RENDER16B
	memset(Retro_Screen,0xff/*0*/,1280*1024*2);
#else
	memset(Retro_Screen,0,1280*1024*2*2);
#endif
	memset(SNDBUF,0,1024*2*2);

#ifndef NO_LIBCO
	co_switch(emuThread);
#else

#endif
   return true;
}

void retro_unload_game(void){

   pauseg=-1;
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void) {}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

