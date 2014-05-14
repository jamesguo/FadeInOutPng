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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <zlib.h>

#define TAG "James::JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG, __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define SEED 509
unsigned char pngData[512 * 512 * 4] = { 0 };
int alpha = 255;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
} rgba;
void process_png(char *file_name) {
	char header[8];
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	int number_of_passes;
	int x, y;
	FILE *fp;
	LOGE("enter process_png");
	if ((fp = fopen(file_name, "rb")) == NULL) {
		LOGE("open file fail");
		return;
	}

	if (png_sig_cmp(header, 0, 8))
		LOGE("[read_png_file] File %s is not recognized as a PNG file", file_name);

	/* initialize stuff */
	LOGE("enter png_create_read_struct");
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
		LOGE("[read_png_file] png_create_read_struct failed");

	LOGE("enter png_create_info_struct");
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		LOGE("[read_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
		LOGE("[read_png_file] Error during init_io");

	LOGE("enter png_init_io");
	png_init_io(png_ptr, fp);
	LOGE("enter png_set_sig_bytes");
	png_set_sig_bytes(png_ptr, 8);

	LOGE("enter png_read_info");
	png_read_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	LOGE("png width = %d,height = %d", width, height);

	number_of_passes = png_set_interlace_handling(png_ptr);
	LOGE("enter png_read_update_info");
	png_read_update_info(png_ptr, info_ptr);

	/* read file */
	if (setjmp(png_jmpbuf(png_ptr)))
		LOGE("[read_png_file] Error during read_image");
	png_bytep* row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (y = 0; y < height; y++)
		row_pointers[y] = (png_byte*) malloc(
				png_get_rowbytes(png_ptr, info_ptr));
	LOGE("enter png_read_image");
	png_read_image(png_ptr, row_pointers);
	fclose(fp);
	unsigned char rgba[512 * 512 * 4] = { 0 };
	int pos = 0;
	int row = 0;
	int col = 0;
	for (row = 0; row < height; row++) {
		for (col = 0; col < (4 * width); col += 4) {
			rgba[pos++] = row_pointers[row][col]; // red
//			LOGE("red value %d",row_pointers[row][col]);
			rgba[pos++] = row_pointers[row][col + 1]; // green
//			LOGE("green value %d",row_pointers[row][col + 1]);
			rgba[pos++] = row_pointers[row][col + 2]; // blue
//			LOGE("blue value %d",row_pointers[row][col + 2]);
			rgba[pos++] = row_pointers[row][col + 3]; // alpha
//			LOGE("alpha value %d",row_pointers[row][col + 3]);
		}
	}
////	LOGE("enter memset");
////	memset(pngData, width * height * 4, sizeof(unsigned char*));
//	LOGE("enter memcpy");
	memcpy(pngData, rgba, 512 * 512 * 4);
	free(row_pointers);
//	LOGE("enter png_destroy_read_struct");
//	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
//	fclose(fp);
}

JNIEXPORT void Java_com_example_fadeinoutpng_MainActivity_loadImage(
		JNIEnv * env, jobject obj, jstring inputfile) {
	const char *str = (*env)->GetStringUTFChars(env, inputfile, 0);
	LOGE("%s", str);
	process_png(str);
	(*env)->ReleaseStringUTFChars(env, inputfile, str);
}

JNIEXPORT void Java_com_example_fadeinoutpng_MainActivity_startFade(
		JNIEnv * env, jobject obj, jobject bitmap) {
	AndroidBitmapInfo info;
	int ret;
	void* pixelsIn;
	if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return;
	}
	if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
		LOGE("Bitmap format is not RGBA_8888 !");
		return;
	}
//	if (info.format != ANDROID_BITMAP_FORMAT_RGB_565) {
//			LOGE("Bitmap format is not RGB_565!");
//			return;
//	}
	if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixelsIn)) < 0) {
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}
	int pos = 0;
	for (pos = 0; pos < 512 * 512; pos++) {
		((rgba*) pixelsIn)[pos].red = pngData[pos * 4];
//		LOGE("red %d", outIn[pos].red);
		((rgba*) pixelsIn)[pos].green = pngData[pos * 4 + 1];
		((rgba*) pixelsIn)[pos].blue = pngData[pos * 4 + 2];
		((rgba*) pixelsIn)[pos].alpha = alpha%SEED<=255?(alpha%SEED):510-(alpha%SEED);

	}
	LOGE("current alpha=%d", ((rgba*) pixelsIn)[0].alpha);
	alpha = (alpha+5)%SEED;
	AndroidBitmap_unlockPixels(env, bitmap);

}
