#include "sysconfig.h"
#include "sysdeps.h"
#include <string>
#include <list>
#include <stdio.h>
#include <sstream>
#include <ctype.h>

#include "libretro/retrodep/WHDLoad_files.zip.c"
#include "libretro/retrodep/WHDLoad_hdf.gz.c"
#include "libretro/retrodep/WHDSaves_hdf.gz.c"
#include "libretro/retrodep/WHDLoad_prefs.gz.c"
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
#include "zlib.h"
#include "zfile.h"
#include "filesys.h"
#include "fsdb.h"
#include "disk.h"
#include "commonvar.h"
#if defined(__LIBRETRO__)
#include "sd-retro/sound.h"
#else
#include "sd-pandora/sound.h"
#endif


#include "libretro.h"

#include "libretro-core.h"

#ifdef _WIN32
#include <tchar.h>
#ifdef UNICODE
#define SIZEOF_TCHAR 2
#else
#define SIZEOF_TCHAR 1
#endif
#else
typedef char TCHAR;
#define SIZEOF_TCHAR 1
#endif

#ifndef _T
#if SIZEOF_TCHAR == 1
#define _T(x) x
#else
#define _T(x) Lx
#endif
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif




#ifndef NO_LIBCO
cothread_t mainThread;
cothread_t emuThread;
#else
//extern void quit_vice_emu();
#endif

int CROP_WIDTH;
int CROP_HEIGHT;
int VIRTUAL_WIDTH ;
int retrow=320; 
int retroh=240;

#define RETRO_DEVICE_AMIGA_KEYBOARD RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_KEYBOARD, 0)
#define RETRO_DEVICE_AMIGA_JOYSTICK RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)

int autorun=0;

unsigned amiga_devices[ 2 ];



extern int SHIFTON,pauseg,SND ,snd_sampler;
extern short signed int SNDBUF[1024*2];
extern char RPATH[512];


extern char RETRO_DIR[512];
extern int SHOWKEY;

#include "cmdline.cpp"

extern void update_input(void);
extern void texture_init(void);
extern void texture_uninit(void);
extern void Emu_init();
extern void Emu_uninit();
extern void input_gui(void);
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

unsigned int opt_use_whdload = 1;
//apiso
#define EMULATOR_DEF_WIDTH 320
#define EMULATOR_DEF_HEIGHT 240

// Amiga default models
// Amiga default kickstarts

#define A500_ROM        "kick34005.A500"
#define A600_ROM        "kick40063.A600"
#define A1200_ROM       "kick40068.A1200"

static char uae_machine[256];
static char uae_kickstart[16];
static char uae_config[1024];										   


#ifdef _WIN32
#define RETRO_PATH_SEPARATOR            "\\"
// Windows also support the unix path separator
#define RETRO_PATH_SEPARATOR_ALT        "/"
#else
#define RETRO_PATH_SEPARATOR            "/"
#endif

void path_join(char* out, const char* basedir, const char* filename)
{
   snprintf(out, MAX_PATH, "%s%s%s", basedir, RETRO_PATH_SEPARATOR, filename);
}

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
     //{ "uae4arm_autorun","Autorun; disabled|enabled" , },
	 { "uae4arm_model", "Model; Auto|A500|A600|A1200", },
     { "uae4arm_fastmem", "Chip Memory Size; Default|512K|1M|2M|4M", },
	 {
         "uae4arm_resolution",
         "Internal resolution; 320x200|320x240|320x256|320x262|320x270|640x200|640x240|640x256|640x262|640x270|768x270|384x224",
  	
      },
	  { "uae4arm_heightmultiplier","Resolution Height Multiplier; Disabled|0.80|0.82|0.84|0.86|0.88|0.90|0.92|0.94|0.96|0.98|1.02|1.04|1.06|1.08|1.10|1.12|1.14|1.16|1.18|1.20|1.22", },
      { "uae4arm_widthmultiplier","Resolution Width Multiplier; Disabled|0.80|0.82|0.84|0.86|0.88|0.90|0.92|0.94|0.96|0.98|1.02|1.04|1.06|1.08|1.10|1.12|1.14|1.16|1.18|1.20|1.22", },
     
	  
	/* {
         "uae4arm_winresolution",
         "Windows resolution; 320x240|320x256|320x262|640x240|640x256|640x262|640x270|768x270|384x224",
  	
      },
	  {
         "uae4arm_fsresolution",
         "Fullscreen resolution; 320x240|320x256|320x262|640x240|640x256|640x262|640x270|768x270|384x224",
  	
      },*/
	 
	 { "uae4arm_autoloadstate","AutoLoad State; Disabled|1|2|3|4|5|6|7|8|9|10|11|12", },
     //{ "puae_analog","Use Analog; OFF|ON", },
     { "uae4arm_leds_on_screen", "Leds on screen; on|off", },
	 { "uae4arm_cpu_speed", "CPU speed; real|max", },
     { "uae4arm_cpu_compatible", "CPU compatible; true|false", },
     { "uae4arm_immediate_blits", "Immediate blit; false|true", },
     { "uae4arm_fast_copper", "Fast Copper; false|true", },
	 { "uae4arm_collision", "Collision level; none|sprites|playfields|full", },
     //{ "uae4arm_sound_mode", "Sound mode; SND_STEREO|SND_MONO|SND_4CH_CLONEDSTEREO|SND_4CH|SND_6CH_CLONEDSTEREO|SND_6CH|SND_NONE", },
	 //{ "uae4arm_sound_output", "Sound output; normal|exact|interrupts|none", },
     //{ "uae4arm_sound_frequency", "Sound frequency; 44100|22050|11025", },
     //{ "uae4rm_sound_interpol", "Sound interpolation; none|rh|crux|sync", },
     { "uae4arm_floppy_speed", "Floppy speed; 100|200|300|400|500|600|700|800", },
     { "uae4arm_framerate", "Frameskipping; 0|1", },
     { "uae4arm_ntsc", "NTSC Mode; false|true", },
     { "uae4arm_refreshrate", "Chipset Refresh Rate; 50|60", },
     //{ "puae_gfx_linemode", "Line mode; double|none", }, // Removed scanline, we have shaders for this
     //{ "uae4arm_gfx_correct_aspect", "Correct aspect ratio; true|false", },
     //{ "puae_gfx_center_vertical", "Vertical centering; simple|smart|none", },
     //{ "puae_gfx_center_horizontal", "Horizontal centering; simple|smart|none", },
	 { "uae4arm_whdloadmode",    "whdload mode; files|hdfs"},
	 { NULL, NULL },
   };
   cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

void gz_uncompress(gzFile in, FILE *out)
{
   char gzbuf[16384];
   int len;
   int err;

   for (;;)
   {
      len = gzread(in, gzbuf, sizeof(gzbuf));
      if (len < 0)
         fprintf(stdout, "%s", gzerror(in, &err));
      if (len == 0)
         break;
      if ((int)fwrite(gzbuf, 1, (unsigned)len, out) != len)
         fprintf(stdout, "Write error!\n");
   }
}


#include "archivers/zip/unzip.h"
//#include "file/file_path.h"
#include "zfile.h"
#ifdef WIN32
#define DIR_SEP_STR "\\"
#else
#define DIR_SEP_STR "/"
#endif
void zip_uncompress(char *in, char *out, char *lastfile)
{
    unzFile uf = NULL;

    struct zfile *zf;
    zf = zfile_fopen(in, _T("rb"));

    uf = unzOpen(zf);

    uLong i;
    unz_global_info gi;
    int err;
    err = unzGetGlobalInfo (uf, &gi);

    const char* password = NULL;
    int size_buf = 8192;

    for (i = 0; i < gi.number_entry; i++)
    {
        char filename_inzip[256];
        char filename_withpath[512];
        filename_inzip[0] = '\0';
        filename_withpath[0] = '\0';
        char* filename_withoutpath;
        char* p;
        unz_file_info file_info;
        FILE *fout = NULL;
        void* buf;

        buf = (void*)malloc(size_buf);
        if (buf == NULL)
        {
            fprintf(stderr, "Unzip: Error allocating memory\n");
            return;
        }

        err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
        snprintf(filename_withpath, sizeof(filename_withpath), "%s%s%s", out, DIR_SEP_STR, filename_inzip);
        //if (dc_get_image_type(filename_inzip) == DC_IMAGE_TYPE_FLOPPY && lastfile != NULL)
        //    snprintf(lastfile, RETRO_PATH_MAX, "%s", filename_inzip);

        p = filename_withoutpath = filename_inzip;
        while ((*p) != '\0')
        {
            if (((*p) == '/') || ((*p) == '\\'))
                filename_withoutpath = p+1;
            p++;
        }

        if ((*filename_withoutpath) == '\0')
        {
            fprintf(stdout, "Unzip mkdir:   %s\n", filename_withpath);
            my_mkdir(filename_withpath);
        }
        else
        {
            const char* write_filename;
            int skip = 0;

            write_filename = filename_withpath;

            err = unzOpenCurrentFile(uf);
            if (err != UNZ_OK)
            {
                fprintf(stderr, "Unzip: Error %d with zipfile in unzOpenCurrentFilePassword: %s\n", err, write_filename);
            }

            if ((skip == 0) && (err == UNZ_OK))
            {
                fout = fopen(write_filename, "wb");
                if (fout == NULL)
                {
                    fprintf(stderr, "Unzip: Error opening %s\n", write_filename);
                }
            }

            if (fout != NULL)
            {
                fprintf(stdout, "Unzip extract: %s\n", write_filename);

                do
                {
                    err = unzReadCurrentFile(uf, buf, size_buf);
                    if (err < 0)
                    {
                        fprintf(stderr, "Unzip: Error %d with zipfile in unzReadCurrentFile\n", err);
                        break;
                    }
                    if (err > 0)
                    {
                        if (!fwrite(buf, err, 1, fout))
                        {
                            fprintf(stderr, "Unzip: Error in writing extracted file\n");
                            err = UNZ_ERRNO;
                            break;
                        }
                    }
                }
                while (err > 0);
                if (fout)
                    fclose(fout);
            }

            if (err == UNZ_OK)
            {
                err = unzCloseCurrentFile(uf);
                if (err != UNZ_OK)
                {
                    fprintf(stderr, "Unzip: Error %d with zipfile in unzCloseCurrentFile\n", err);
                }
            }
            else
                unzCloseCurrentFile(uf);
        }

        free(buf);

        if ((i+1) < gi.number_entry)
        {
            err = unzGoToNextFile(uf);
            if (err != UNZ_OK)
            {
                fprintf(stderr, "Unzip: Error %d with zipfile in unzGoToNextFile\n", err);
                break;
            }
        }
    }

    if (uf)
    {
        unzCloseCurrentFile(uf);
        unzClose(uf);
        uf = NULL;
    }
}



void update_prefs_retrocfg(struct uae_prefs * prefs, bool loadlha)
{
   uae_machine[0] = '\0';
   uae_config[0]  = '\0';

   struct retro_variable var = {0};

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

	  float multiplierw=1;
	  float multiplierh=1;

	  //int retrowfs,retrohfs,retrowwin,retrohwin;
	  struct retro_variable varmul = {0};
	  varmul.key = "uae4arm_heightmultiplier";
	  varmul.value = NULL;	  
	  
	  
	  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &varmul) && varmul.value)
	  {
		    if (strcmp(varmul.value, "Disabled") == 0)
		  {
			  multiplierh=1;
		  }
		  else
		  {
			//Implementazione dell'autoload dello state sullo slot 12, serve   
			multiplierh = atof(varmul.value) ;
			  
		  }
	  }

 struct retro_variable varmulw = {0};
	  varmulw.key = "uae4arm_widthmultiplier";
	  varmulw.value = NULL;	  
	  
	  
	  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &varmulw) && varmulw.value)
	  {
		    if (strcmp(varmulw.value, "Disabled") == 0)
		  {
			  multiplierw=1;
		  }
		  else
		  {
			//Implementazione dell'autoload dello state sullo slot 12, serve   
			multiplierw = atof(varmulw.value) ;
			  
		  }
	  }
	  
/*	  int retrowfs,retrohfs,retrowwin,retrohwin;
	  struct retro_variable varwin = {0};
	  varwin.key = "uae4arm_winresolution";
	  varwin.value = NULL;	  
	  if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &varwin) && varwin.value)
	  {
		  char *pchwin;
		  char str[100];
		  snprintf(str, sizeof(str), varwin.value);

		  pch = strtok(str, "x");
		  if (pch)
			 retrowwin = strtoul(pch, NULL, 0);
		  pch = strtok(NULL, "x");
		  if (pch)
			 retrohwin = strtoul(pch, NULL, 0);
	  }
	  else
	  {
		   retrowwin=retrow;
		   retrohwin=retroh;
	  }
	  
	  struct retro_variable varfs = {0}; 
	  varfs.key = "uae4arm_fsresolution";
	  varfs.value = NULL;	  
	 if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &varfs) && varfs.value)
	  {
		  char *pchwin;
		  char str[100];
		  snprintf(str, sizeof(str), varfs.value);

		  pch = strtok(str, "x");
		  if (pch)
			 retrowfs = strtoul(pch, NULL, 0);
		  pch = strtok(NULL, "x");
		  if (pch)
			 retrohfs = strtoul(pch, NULL, 0);
	  }
	  else
	  {
		   retrohfs=retrow;
		   retrowfs=retroh;
	  }
	  prefs->gfx_size_win.width = retrowwin;
	  prefs->gfx_size_win.height = retrowwin;
	  prefs->gfx_size_fs.width = retrowfs;
	  prefs->gfx_size_fs.height = retrohfs;
  */
	  retrow= (int)(retrow * multiplierw);
	  retroh= (int)(retroh * multiplierh);
	
	  
      prefs->gfx_size.width  = retrow ;
      prefs->gfx_size.height = retroh ;
	  
	 
      prefs->gfx_resolution  = prefs->gfx_size.width > 600 ? 1 : 0;

      LOGI("[libretro-uae4arm]: Got size: %u x %u.\n", retrow, retroh);

      CROP_WIDTH =retrow;
      CROP_HEIGHT= (retroh-80);
      VIRTUAL_WIDTH = retrow;
      texture_init();

   }

   var.key = "uae4arm_autorun";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
     if (strcmp(var.value, "enabled") == 0)
			 autorun = 1;
   }
   
   var.key = "uae4arm_leds_on_screen";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "on") == 0)  prefs->leds_on_screen = 1;
      if (strcmp(var.value, "off") == 0) prefs->leds_on_screen = 0;
   }

  var.key = "uae4arm_fastmem";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "None") == 0)
      {
         prefs->fastmem_size = 0;
      }
      if (strcmp(var.value, "1 MB") == 0)
      {
         prefs->fastmem_size = 0x100000;
      }
      if (strcmp(var.value, "2 MB") == 0)
      {
         prefs->fastmem_size = 0x100000 * 2;
      }
      if (strcmp(var.value, "4 MB") == 0)
      {
         prefs->fastmem_size = 0x100000 * 4;
      }
      if (strcmp(var.value, "8 MB") == 0)
      {
         prefs->fastmem_size = 0x100000 * 8;
      }
   }
   
   var.key = "uae4arm_floppy_speed";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	    prefs->floppy_speed=atoi(var.value);
   }
   
   var.key = "uae4arm_whdloadmode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "hdfs") == 0)
      {
         opt_use_whdload = 2;
      }
      else
      {
         opt_use_whdload = 1;
      }
   }
   
   var.key = "uae4arm_model";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      LOGI("[libretro-uae4arm]: Got model: %s.\n", var.value);
      if (strcmp(var.value, "Auto") == 0)
      {
         if (strcasestr(RPATH,"aga") != NULL)
         {
            LOGI("[libretro-uae4arm]: Auto-model -> A1200 selected\n");
            var.value = "A1200";
         }
         else
         {
            if ((strcasestr(RPATH,".hdf") != NULL) || (strcasestr(RPATH,".lha") != NULL))
            {
               if (strcasestr(RPATH,"cd32") != NULL)
               {
                  // Some whdload have cd32 in their name...
                  LOGI("[libretro-uae4arm]: Auto-model -> A1200 selected\n");
                  var.value = "A1200";
               }
               else
               {
                  LOGI("[libretro-uae4arm]: Auto-model -> A600 selected\n");
                  var.value = "A600";
               }
            }
            else
            {
               LOGI("[libretro-uae4arm]: Auto-model -> A500 selected\n");
               var.value = "A500";
            }
         }
      }



   if (strcasestr(RPATH,".lha") != NULL && loadlha )
   {
	   
	   char whdload_hdf[PATH_MAX] = {0};
          char tmp_str [PATH_MAX];
	    struct uaedev_config_info *uci;

	    // WHDLoad HDF mode
        if (opt_use_whdload == 2)
        {
		   path_join((char*)&whdload_hdf, retro_save_directory, "WHDLoad.hdf");

          /* Verify WHDLoad.hdf */
          if (!my_existsfile(whdload_hdf))
          {
             LOGI("[libretro-uae4arm]: WHDLoad image file '%s' not found, attempting to create one\n", (const char*)&whdload_hdf);

             char whdload_hdf_gz[512];
             path_join((char*)&whdload_hdf_gz, retro_save_directory, "WHDLoad.hdf.gz");

             FILE *whdload_hdf_gz_fp;
             if ((whdload_hdf_gz_fp = fopen(whdload_hdf_gz, "wb")))
             {
                /* Write GZ */
                fwrite(___whdload_WHDLoad_hdf_gz, ___whdload_WHDLoad_hdf_gz_len, 1, whdload_hdf_gz_fp);
                fclose(whdload_hdf_gz_fp);

                /* Extract GZ */
                struct gzFile_s *whdload_hdf_gz_fp;
                if ((whdload_hdf_gz_fp = gzopen(whdload_hdf_gz, "r")))
                {
                   FILE *whdload_hdf_fp;
                   if ((whdload_hdf_fp = fopen(whdload_hdf, "wb")))
                   {
                      gz_uncompress(whdload_hdf_gz_fp, whdload_hdf_fp);
                      fclose(whdload_hdf_fp);
                   }
                   gzclose(whdload_hdf_gz_fp);
                }
                remove(whdload_hdf_gz);
             }
             else
                LOGI("[libretro-uae4arm]: Unable to create WHDLoad image file: '%s'\n", (const char*)&whdload_hdf);
          }

          /* Attach HDF */
          if (my_existsfile(whdload_hdf))
          {
              //uaedev_config_info ci;
              struct uaedev_config_info *uci;

              LOGI("[libretro-uae4arm]: Attach HDF\n");

              uci = add_filesys_config(prefs, -1, "WHDLoad", 0, whdload_hdf, 0, 
                    32, 1, 2, 512, 0, 0, 0, 0);

              if (uci)
                  hardfile_do_disk_change (uci, 1);
          }
	  }
		  //WHD File mode
		  else
		  {
			 char whdload_path[PATH_MAX];
             path_join((char*)&whdload_path, retro_save_directory, "WHDLoad");

             char whdload_c_path[PATH_MAX];
             path_join((char*)&whdload_c_path, retro_save_directory, "WHDLoad/C");

             if (!my_existsdir(whdload_path) || (my_existsdir(whdload_path) && !my_existsdir(whdload_c_path)))
             {
                LOGI("[libretro-uae4arm]: WHDLoad image directory '%s' not found, attempting to create one\n", (const char*)&whdload_path);
                my_mkdir(whdload_path);

                char whdload_files_zip[PATH_MAX];
                path_join((char*)&whdload_files_zip, retro_save_directory, "WHDLoad_files.zip");

                FILE *whdload_files_zip_fp;
                if (whdload_files_zip_fp = fopen(whdload_files_zip, "wb"))
                {
                   // Write ZIP
                   fwrite(___whdload_WHDLoad_files_zip, ___whdload_WHDLoad_files_zip_len, 1, whdload_files_zip_fp);
                   fclose(whdload_files_zip_fp);

                   // Extract ZIP
                   zip_uncompress(whdload_files_zip, whdload_path, NULL);
                   remove(whdload_files_zip);
                }
                else
                   LOGI("[libretro-uae4arm]: Error extracting WHDLoad directory: '%s'!\n", (const char*)&whdload_path);
             }
             if (my_existsdir(whdload_path) && my_existsdir(whdload_c_path))
             {
                LOGI("[libretro-uae4arm]: Attach WHDLoad files\n");
                //tmp_str = string_replace_substring(whdload_path, "\\", "\\\\");
                //fprintf(configfile, "filesystem2=rw,WHDLoad:WHDLoad:\"%s\",0\n", (const char*)tmp_str);
                //free(tmp_str);
                //tmp_str = NULL;

			   uci = add_filesys_config(prefs, -1, "WHDLoad", "WHDLoad", whdload_path, 0, 0, 0, 0, 0, 0, 0, 0, 0);
               if (uci) filesys_media_change (uci->rootdir, 1, uci);
		  
             }
             else
                LOGI("[libretro-uae4arm]: Error creating WHDLoad directory: '%s'!\n", (const char*)&whdload_path);
		  } 
		  
		  // Attach retro_system_directory as a read only hard drive for WHDLoad kickstarts/prefs/key
          LOGI("[libretro-uae4arm]: Attach whdcommon\n");

          // Force the ending slash to make sure the path is not treated as a file
          if (retro_system_directory[strlen(retro_system_directory) - 1] != '/')
             sprintf(tmp_str,"%s%s",retro_system_directory, "/");
          else
             sprintf(tmp_str,"%s",retro_system_directory);

          uci = add_filesys_config(prefs, -1, "whdcommon", "", tmp_str, 
            0, 0, 0, 0, 0, -128, 0, 0, 0);

          if (uci)
		  filesys_media_change (uci->rootdir, 1, uci);
	 
	     
		  if (opt_use_whdload == 2)
          {
		   char whdsaves_hdf[MAX_DPATH];
				// Attach WHDSaves.hdf if available
             
             path_join((char*)&whdsaves_hdf, retro_system_directory, "WHDSaves.hdf");
             if (!my_existsfile(whdsaves_hdf))
                path_join((char*)&whdsaves_hdf, retro_save_directory, "WHDSaves.hdf");
             if (!my_existsfile(whdsaves_hdf))
             {
                fprintf(stdout, "[libretro-uae4arm]: WHDSaves image file '%s' not found, attempting to create one\n", (const char*)&whdsaves_hdf);

                char whdsaves_hdf_gz[MAX_DPATH];
                path_join((char*)&whdsaves_hdf_gz, retro_save_directory, "WHDSaves.hdf.gz");

                FILE *whdsaves_hdf_gz_fp;
                if (whdsaves_hdf_gz_fp = fopen(whdsaves_hdf_gz, "wb"))
                {
                   // Write GZ
                   fwrite(___whdload_WHDSaves_hdf_gz, ___whdload_WHDSaves_hdf_gz_len, 1, whdsaves_hdf_gz_fp);
                   fclose(whdsaves_hdf_gz_fp);

                   // Extract GZ
                   struct gzFile_s *whdsaves_hdf_gz_fp;
                   if (whdsaves_hdf_gz_fp = gzopen(whdsaves_hdf_gz, "r"))
                   {
                      FILE *whdsaves_hdf_fp;
                      if (whdsaves_hdf_fp = fopen(whdsaves_hdf, "wb"))
                      {
                         gz_uncompress(whdsaves_hdf_gz_fp, whdsaves_hdf_fp);
                         fclose(whdsaves_hdf_fp);
                      }
                      gzclose(whdsaves_hdf_gz_fp);
                   }
                   remove(whdsaves_hdf_gz);
                }
                else
                   fprintf(stderr, "Error creating WHDSaves.hdf: '%s'!\n", (const char*)&whdsaves_hdf);
             }
			 if (my_existsfile(whdsaves_hdf))
			 {
			   //uaedev_config_info ci;
			   struct uaedev_config_info *uci;
			   LOGI("[libretro-uae4arm]: Attach HDF\n");
			   uci = add_filesys_config(prefs, -1, "WHDSaves", 0, whdsaves_hdf, 0, 32, 1, 2, 512, 0, 0, 0, 0);
			   if (uci)
				  hardfile_do_disk_change (uci, 1);
			 }

          }
		  // WHDSaves file mode
		  else
	      {
			 char whdsaves_path[MAX_DPATH];
             path_join((char*)&whdsaves_path, retro_save_directory, "WHDSaves");
			  if (!my_existsdir(whdsaves_path))
                my_mkdir(whdsaves_path);
             if (my_existsdir(whdsaves_path))
             {
				  //Metto parametri ad archive
				  uci = add_filesys_config(prefs, -1, "WHDSaves", "WHDSaves", whdsaves_path, 
					0, 0, 0, 0, 0, 0, 0, 0, 0);

				  if (uci) filesys_media_change (uci->rootdir, 1, uci);
			  }
			   else
                fprintf(stderr, "Error creating WHDSaves directory: '%s'!\n", (const char*)&whdsaves_path);
  
		  }	

		   /* Attach LHA */

          LOGI("[libretro-uae4arm]: Attach LHA\n");

          uci = add_filesys_config(prefs, -1, "DH0", "LHA", RPATH, 
            0, 0, 0, 0, 0, -128, 0, 0, 0);
          if (uci)
              filesys_media_change (uci->rootdir, -1, uci);
		  
		  // Temp: Add automatically 8 MBytes of Fast...
          prefs->fastmem_size= 0x100000 * 8;
	   }	   
   

      if (strcmp(var.value, "A600") == 0)
      {
         //strcat(uae_machine, A600);
         //strcpy(uae_kickstart, A600_ROM);
         prefs->cpu_model = 68000;
         prefs->chipmem_size = 2 * 0x80000;
         prefs->m68k_speed = M68K_SPEED_7MHZ_CYCLES;
         prefs->cpu_compatible = 0;
         prefs->address_space_24 = 1;
         prefs->chipset_mask = CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS;
         //strcpy(prefs->romfile, A600_ROM);
         path_join(prefs->romfile, retro_system_directory, A600_ROM);
      }
      else if (strcmp(var.value, "A1200") == 0)
      {
         //strcat(uae_machine, A1200);
         //strcpy(uae_kickstart, A1200_ROM);
         //prefs->cpu_type="68ec020";
         prefs->cpu_model = 68020;
         prefs->chipmem_size = 4 * 0x80000;
         prefs->m68k_speed = M68K_SPEED_14MHZ_CYCLES;
         prefs->cpu_compatible = 0;
         prefs->address_space_24 = 1;
         prefs->chipset_mask = CSMASK_AGA | CSMASK_ECS_DENISE | CSMASK_ECS_AGNUS;
         //strcpy(prefs->romfile, A1200_ROM);
         path_join(prefs->romfile, retro_system_directory, A1200_ROM);
      }
      else // if (strcmp(var.value, "A500") == 0)
      {
         //strcat(uae_machine, A500);
         //strcpy(uae_kickstart, A500_ROM);
         //prefs->cpu_type="68000";
         
         prefs->cpu_model = 68000;
         prefs->m68k_speed = M68K_SPEED_7MHZ_CYCLES;
         prefs->cpu_compatible = 0;
         prefs->chipmem_size = 2 * 0x80000;
         prefs->address_space_24 = 1;
         prefs->chipset_mask = CSMASK_ECS_AGNUS;
         //strcpy(prefs->romfile, A500_ROM);
         path_join(prefs->romfile, retro_system_directory, A500_ROM);
      }
   }


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
   
  
   var.key = "uae4arm_collision";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "none") == 0)
      {
        prefs->collision_level = 0;        
      }
      if (strcmp(var.value, "sprites") == 0)
      {
        prefs->collision_level = 1;  
      }
	     if (strcmp(var.value, "playfields") == 0)
      {
        prefs->collision_level = 2;  
      }
	     if (strcmp(var.value, "full") == 0)
      {
        prefs->collision_level = 3;  
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
			prefs->m68k_speed = -1;
		}
		else
		{
			prefs->m68k_speed = 0;
		}
	  
   }

   var.key = "uae4arm_cpu_compatible";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		//strcat(uae_config, "cpu_compatible=");
		if (strcmp(var.value, "true") == 0)
		{
			prefs->cpu_compatible = 1;
		}
		else
		{
			prefs->cpu_compatible = 0;
		}
		//strcat(uae_config, var.value);
		//strcat(uae_config, "\n");
   }

   var.key = "uae4arm_fast_copper";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		if (strcmp(var.value, "true") == 0)
		{
			prefs->fast_copper = 1;
		}
		else
		{
			prefs->fast_copper = 0;
		}
   }

   var.key = "uae4arm_immediate_blits";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		if (strcmp(var.value, "true") == 0)
		{
			prefs->immediate_blits = 1;
		}
		else
		{
			prefs->immediate_blits = 0;
		}
   }
 
   var.key = "uae4arm_framerate";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	    prefs->gfx_framerate=atoi(var.value);
   }

   var.key = "uae4arm_ntsc";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		if (strcmp(var.value, "true") == 0)
		{
			prefs->ntscmode = 1;
		}
		else
		{
			prefs->ntscmode = 0;
		}
   }

   var.key = "uae4arm_refreshrate";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
	    prefs->chipset_refreshrate=atoi(var.value);
   }

  
   var.key = "uae4arm_gfx_correct_aspect";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
		
		if (strcmp(var.value, "true") == 0)
		{
			prefs->gfx_correct_aspect = 1;
		}
		else
		{
			prefs->gfx_correct_aspect = 0;
		}
   }

  


}






static void retro_wrap_emulator()
{    

   pre_main(RPATH);

   LOGI("EXIT EMU THD\n");
   pauseg=-1;

   // Were done here
   co_switch(mainThread);

   // Dead emulator, but libco says not to return
   while(true)
   {
      LOGI("Running a dead emulator.");
      co_switch(mainThread);
   }
}

void Emu_init(){

   memset(Key_Sate,0,512);
   memset(Key_Sate2,0,512);
    if(!emuThread && !mainThread)
   {
      mainThread = co_active();
      emuThread = co_create(8*65536*sizeof(void*), retro_wrap_emulator);
   }
}

void Emu_uninit(){
   texture_uninit();
}

void retro_shutdown_core(void)
{
   LOGI("SHUTDOWN\n");
   texture_uninit();
   environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
}

void retro_reset(void)
{
   uae_reset(1);
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
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "X" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Y" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start"  },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right"  },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left"   },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up"     },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down"   },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "R"  },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,      "L"  },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "R2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,     "L2" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "R3" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,     "L3" }
   };
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &inputDescriptors);

   Emu_init();

   texture_init();

}

extern void main_exit();
void retro_deinit(void)
{
   Emu_uninit(); 

   co_switch(emuThread);
   LOGI("exit emu\n");

   co_switch(mainThread);
   LOGI("exit main\n");
   if(emuThread)
   {
      co_delete(emuThread);
      emuThread = 0;
   }

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
   info->library_version  = "0.31";
   info->valid_extensions = "adf|dms|zip|ipf|hdf|lha|uae";
   info->need_fullpath    = true;
   info->block_extract    = true;
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


void retro_audiocb(signed short int *sound_buffer,int sndbufsize){
   
   if(pauseg==0)
        audio_batch_cb(sound_buffer, sndbufsize);
}

extern void testsnd(int len);

void retro_run(void)
{
   bool updated = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      update_prefs_retrocfg(&changed_prefs,false);

   co_switch(emuThread);

   if(pauseg==0)
      if(SHOWKEY )
         retro_virtualkb();

   video_cb(Retro_Screen,retrow,retroh,retrow<<PIXEL_BYTES);

}

bool retro_load_game(const struct retro_game_info *info)
{
   const char *full_path;

   (void)info;

   full_path = info->path;

   strcpy(RPATH,full_path);

#ifdef RENDER16B
   memset(Retro_Screen,0,1280*1024*2);
#else
   memset(Retro_Screen,0,1280*1024*2*2);
#endif
   memset(SNDBUF,0,1024*2*2);

   co_switch(emuThread);

   return true;
}

void retro_unload_game(void)
{
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

