#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/input.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <time.h>
#include <jni.h>
#include <math.h>

#include <png.h>
#include <pngstruct.h>
#include <pnginfo.h>
#include <pngconf.h>
#include <pnglibconf.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <zlib.h>

#define TAG "James::JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG, __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

unsigned char pngData[512*512*4]={0};
int alpha = 255;
typedef struct {
    double  renderTime;
    double  frameTime;
} FrameStats;

static double now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000. + tv.tv_usec/1000.;
}

#define  MAX_FRAME_STATS  200
#define  MAX_PERIOD_MS    1500
typedef struct {
    double  firstTime;
    double  lastTime;
    double  frameTime;

    int         firstFrame;
    int         numFrames;
    FrameStats  frames[ MAX_FRAME_STATS ];
} Stats;

static void
stats_init( Stats*  s )
{
    s->lastTime = now_ms();
    s->firstTime = 0.;
    s->firstFrame = 0;
    s->numFrames  = 0;
}
static void
stats_startFrame( Stats*  s )
{
    s->frameTime = now_ms();
}

static void
stats_endFrame( Stats*  s )
{
    double now = now_ms();
    double renderTime = now - s->frameTime;
    double frameTime  = now - s->lastTime;
    int nn;

    if (now - s->firstTime >= MAX_PERIOD_MS) {
        if (s->numFrames > 0) {
            double minRender, maxRender, avgRender;
            double minFrame, maxFrame, avgFrame;
            int count;

            nn = s->firstFrame;
            minRender = maxRender = avgRender = s->frames[nn].renderTime;
            minFrame  = maxFrame  = avgFrame  = s->frames[nn].frameTime;
            for (count = s->numFrames; count > 0; count-- ) {
                nn += 1;
                if (nn >= MAX_FRAME_STATS)
                    nn -= MAX_FRAME_STATS;
                double render = s->frames[nn].renderTime;
                if (render < minRender) minRender = render;
                if (render > maxRender) maxRender = render;
                double frame = s->frames[nn].frameTime;
                if (frame < minFrame) minFrame = frame;
                if (frame > maxFrame) maxFrame = frame;
                avgRender += render;
                avgFrame  += frame;
            }
            avgRender /= s->numFrames;
            avgFrame  /= s->numFrames;

            LOGI("frame/s (avg,min,max) = (%.1f,%.1f,%.1f) "
                 "render time ms (avg,min,max) = (%.1f,%.1f,%.1f)\n",
                 1000./avgFrame, 1000./maxFrame, 1000./minFrame,
                 avgRender, minRender, maxRender);
        }
        s->numFrames  = 0;
        s->firstFrame = 0;
        s->firstTime  = now;
    }

    nn = s->firstFrame + s->numFrames;
    if (nn >= MAX_FRAME_STATS)
        nn -= MAX_FRAME_STATS;

    s->frames[nn].renderTime = renderTime;
    s->frames[nn].frameTime  = frameTime;

    if (s->numFrames < MAX_FRAME_STATS) {
        s->numFrames += 1;
    } else {
        s->firstFrame += 1;
        if (s->firstFrame >= MAX_FRAME_STATS)
            s->firstFrame -= MAX_FRAME_STATS;
    }

    s->lastTime = now;
}
void process_png(char *file_name) {
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	FILE *fp;
	LOGE("enter process_png");
	if ((fp = fopen(file_name, "rb")) == NULL) {
		LOGE("open file fail");
		return;
	}
	fseek(fp, 0L, SEEK_SET);
	LOGE("enter png_create_read_struct");
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
			NULL);

	if (png_ptr == NULL) {
		return;
	}
	LOGE("enter png_create_info_struct");
	png_ptr = png_create_info_struct(png_ptr);
	if (png_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &png_ptr, NULL);
		return;
	}
	LOGE("enter png_init_io");
	png_init_io(png_ptr, fp);
//	LOGE("enter png_set_sig_bytes");
//	png_set_sig_bytes(png_ptr, 8);
//	LOGE("enter png_read_png");
//	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);


	LOGE("enter png_read_info");
	png_read_info(png_ptr, info_ptr);
	LOGE("enter png_get_IHDR");
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
			&color_type, &interlace_type, NULL, NULL);

//	LOGE("enter png_read_update_info");
//	png_read_update_info(png_ptr,info_ptr);
	width = info_ptr->width;
	height = info_ptr->height;
	LOGE("png width = %d,height = %d",width,height);
	png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);
//	png_bytep* row_pointers = (png_bytep *) malloc(png_get_rowbytes(png_ptr, info_ptr));

	unsigned char rgba[512*512*4] = { 0 };
//	png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);
	int pos = 0;
	int row = 0;
	int col = 0;
	for (row = 0; row < height; row++) {
		for (col = 0; col < (4 * width); col += 4) {
			rgba[pos++] = row_pointers[row][col]; // red
			LOGE("red value %d",row_pointers[row][col]);
			rgba[pos++] = row_pointers[row][col + 1]; // green
			LOGE("green value %d",row_pointers[row][col + 1]);
			rgba[pos++] = row_pointers[row][col + 2]; // blue
			LOGE("blue value %d",row_pointers[row][col + 2]);
			rgba[pos++] = row_pointers[row][col + 3]; // alpha
			LOGE("alpha value %d",row_pointers[row][col + 3]);
		}
	}
//	LOGE("enter memset");
//	memset(pngData, width * height * 4, sizeof(unsigned char*));
	LOGE("enter memcpy");
	memcpy(pngData, rgba, 512*512*4);
	LOGE("enter png_destroy_read_struct");
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
}

int check_if_png(FILE *fp) {
	if (fp == NULL) {
		return 0;
	}

	const int SIZE = 4;
	char buf[SIZE];

	fseek(fp, 0L, SEEK_SET);

	if (fread(buf, sizeof(char), SIZE, fp) != SIZE) {
		return 0;
	}

	return (!png_sig_cmp(buf, (png_size_t) 0, SIZE));
}
static void  fill_plasma( AndroidBitmapInfo*  info, void*  pixels, double  t)
{

	int totalsize = sizeof(pngData)/sizeof(unsigned char*);
	int pos = 0;
	for (pos=3; pos < totalsize;pos += 4) {
		pngData[pos] = alpha%256;
	}
	alpha++;
}
JNIEXPORT void Java_com_example_fadeinoutpng_MainActivity_loadImage(JNIEnv * env, jobject  obj, jstring inputfile) {
	const char *str = (*env)->GetStringUTFChars(env, inputfile, 0);
	LOGE("%s", str);
	process_png(str);
	(*env)->ReleaseStringUTFChars(env, inputfile, str);
}

JNIEXPORT void Java_com_example_fadeinoutpng_MainActivity_startFade(JNIEnv * env, jobject  obj, jobject bitmap,  jlong  time_ms) {
	AndroidBitmapInfo info;
	int ret;
	static Stats       stats;
	static int         init;
	if (!init) {
		stats_init(&stats);
		init = 1;
	}
	if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return;
	}

	if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
		LOGE("Bitmap format is not RGBA_8888 !");
		return;
	}

	if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pngData)) < 0) {
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}
//	stats_startFrame(&stats);

	/* Now fill the values with a nice little plasma */
//	fill_plasma(&info, pngData, time_ms );

	AndroidBitmap_unlockPixels(env, bitmap);

//	stats_endFrame(&stats);
}
