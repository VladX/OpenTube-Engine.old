/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - VladX; http://vladx.net/
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <pthread.h>
#include <time.h>
#include <glob.h>
#include <png.h>
#include <common_functions.h>
#include "captcha.h"

#define CAPTCHA_MIN_CHARS 5
#define CAPTCHA_MAX_CHARS 7
#define CAPTCHA_WIDTH 160
#define CAPTCHA_HEIGHT 70
#define CAPTCHA_CHANNELS 3
#define CAPTCHA_ROWSIZE 480 /* CAPTCHA_WIDTH * CAPTCHA_CHANNELS */

struct captcha_syms
{
	char sym;
	uchar ** rows;
	uint rowbytes;
	uchar channels;
	ushort width;
	ushort height;
};

static pthread_mutex_t mutex[1] = {PTHREAD_MUTEX_INITIALIZER};

static bool init = false;
static const char * captcha_patterns = "/data/captcha/*.png";
static struct captcha_syms * syms = NULL;
static uint syms_size = 0;
static threadsafe uchar ** blankimg = NULL;
static threadsafe png_structp png_ptr = NULL;
static threadsafe png_infop info_ptr = NULL;
static threadsafe buf_t * outputbuffer = NULL;
static threadsafe uchar * outputkeyword = NULL;

static void png_write_data_cb (png_structp png_ptr, png_bytep data, png_size_t length)
{
	uchar * p = (uchar *) outputbuffer->data + outputbuffer->cur_len;
	buf_expand(outputbuffer, length);
	memcpy(p, data, length);
}

static void libpng_error (png_structp png_ptr, png_const_charp msg)
{
	eerr(0, "libpng error: %s", msg);
}

static void libpng_warn (png_structp png_ptr, png_const_charp msg)
{
	debug_print_1("libpng warning: %s", msg);
}

struct captcha_output * captcha_generate (struct captcha_output * output)
{
	if (syms == NULL)
		return NULL;
	
	buf_free(outputbuffer);
	srand(time(NULL));
	uchar nchars = (rand() % ((CAPTCHA_MAX_CHARS - CAPTCHA_MIN_CHARS) + 1)) + CAPTCHA_MIN_CHARS;
	
	uchar i;
	struct captcha_syms * s;
	
	for (i = 0; i < nchars; i++)
	{
		s = &(syms[rand() % syms_size]);
		outputkeyword[i] = s->sym;
	}
	
	assert(png_ptr != NULL);
	assert(info_ptr != NULL);
	
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, blankimg);
	png_write_end(png_ptr, info_ptr);
	
	debug_print_3("%d", outputbuffer->cur_len);
	
	output->keyword.str = outputkeyword;
	output->keyword.len = nchars;
	output->png_data.str = outputbuffer->data;
	output->png_data.len = outputbuffer->cur_len;
	
	return output;
}

void captcha_init (void)
{
	pthread_mutex_lock(mutex);
	
	if (init == true)
		goto fn_end;
	
	init = true;
	char * captcha_pat = (char *) malloc(config.data.len + strlen(captcha_patterns) + 1);
	strcpy(captcha_pat, (char *) config.data.str);
	strcat(captcha_pat, captcha_patterns);
	
	glob_t globbuf;
	int ret = glob(captcha_pat, 0, NULL, &globbuf);
	assert(ret == 0);
	
	uint i, k, len;
	char * sym;
	FILE * fp;
	
	syms = NULL;
	
	for (i = 0; i < globbuf.gl_pathc; i++)
	{
		len = strlen(globbuf.gl_pathv[i]);
		for (sym = globbuf.gl_pathv[i] + len; * sym != '.'; sym--)
			if (sym < globbuf.gl_pathv[i])
				break;
		sym--;
		if (sym < globbuf.gl_pathv[i])
				continue;
		fp = fopen(globbuf.gl_pathv[i], "rb");
		assert(fp != NULL);
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, (void *) libpng_error, (void *) libpng_warn);
		assert(png_ptr != NULL);
		info_ptr = png_create_info_struct(png_ptr);
		assert(info_ptr != NULL);
		png_init_io(png_ptr, fp);
		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_PACKSWAP, NULL);
		k = syms_size;
		syms_size++;
		syms = (struct captcha_syms *) realloc(syms, syms_size * sizeof(struct captcha_syms));
		syms[k].sym = TO_LOWER(* sym);
		syms[k].rowbytes = png_get_rowbytes(png_ptr,info_ptr);
		syms[k].channels = png_get_channels(png_ptr, info_ptr);
		syms[k].width = png_get_image_width(png_ptr, info_ptr);
		syms[k].height = png_get_image_height(png_ptr, info_ptr);
		syms[k].rows = info_ptr->row_pointers;
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		png_ptr = NULL;
		info_ptr = NULL;
		fclose(fp);
	}
	
	globfree(&globbuf);
	free(captcha_pat);
	
	fn_end:
	
	if (blankimg == NULL)
	{
		blankimg = (uchar **) malloc(sizeof(uchar *) * CAPTCHA_HEIGHT);
		blankimg[0] = (uchar *) malloc(CAPTCHA_ROWSIZE * CAPTCHA_HEIGHT);
		memset(blankimg[0], 0xFF, CAPTCHA_ROWSIZE * CAPTCHA_HEIGHT);
		for (i = 0; i < CAPTCHA_HEIGHT; i++)
			blankimg[i] = (blankimg[0]) + i * CAPTCHA_ROWSIZE;
	}
	
	if (outputbuffer == NULL)
	{
		outputbuffer = buf_create(1, 1);
		outputkeyword = (uchar *) malloc(CAPTCHA_MAX_CHARS + 1);
		memset(outputkeyword, 0, CAPTCHA_MAX_CHARS + 1);
	}
	
	if (png_ptr == NULL)
	{
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, (void *) libpng_error, (void *) libpng_warn);
		assert(png_ptr != NULL);
		info_ptr = png_create_info_struct(png_ptr);
		assert(info_ptr != NULL);
		png_set_write_fn(png_ptr, NULL, (void *) png_write_data_cb, NULL);
		png_set_IHDR(png_ptr, info_ptr, CAPTCHA_WIDTH, CAPTCHA_HEIGHT, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		
		png_color_8 sig_bit;
		sig_bit.red = 8;
		sig_bit.green = 8;
		sig_bit.blue = 8;
		
		png_set_sBIT(png_ptr, info_ptr, &sig_bit);
	}
	
	pthread_mutex_unlock(mutex);
}
