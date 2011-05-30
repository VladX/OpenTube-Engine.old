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
#ifndef _MSC_VER
 #include <sys/time.h>
#endif
#include <glob.h>
#include <math.h>
#include <common_functions.h>
#include <endianness.h>
#include <win32_utils.h>
#include "captcha.h"

#ifndef M_PI
 #define M_PI 3.14159265358979323846
#endif

#define CAPTCHA_MIN_CHARS 5
#define CAPTCHA_MAX_CHARS 7
#define CAPTCHA_WIDTH 200
#define CAPTCHA_HEIGHT 70
#define CAPTCHA_CHANNELS 3
#define CAPTCHA_ROWSIZE 600 /* CAPTCHA_WIDTH * CAPTCHA_CHANNELS */
#define CAPTCHA_X_PADDING 8
#define CAPTCHA_Y_PADDING 8
#define CAPTCHA_ROTATE_ANGLE 45
#define CAPTCHA_NOISE 2500
#define CAPTCHA_WAVES_AMPLITUDE 1.32
#define CAPTCHA_WAVES_LENGTH 15.0

struct captcha_syms
{
	char sym;
	uchar * rows;
	uint rowbytes;
	uchar channels;
	ushort width;
	ushort height;
};

static pthread_mutex_t mutex[1] = {PTHREAD_MUTEX_INITIALIZER};

static bool init = false;
static const char * captcha_patterns = "/data/captcha/*.ppm";
static struct captcha_syms * syms = NULL;
static uint syms_size = 0;
static threadsafe uchar ** blankimg = NULL;
static threadsafe uchar * outimg = NULL;
static threadsafe buf_t * outputbuffer = NULL;
static threadsafe uchar * outputkeyword = NULL;
static threadsafe z_stream zstrm;

#if __BYTE_ORDER == __BIG_ENDIAN

static uint png_header[] = {
	2303741511, 218765834, 13, 1229472850, CAPTCHA_WIDTH, CAPTCHA_HEIGHT, 134348800,
	4160200, 4043309056, 57885257, 1409812488, 3688976352, 0, 1229209940
};

static uint png_trailer[] = {0x00000000, 0x00000000, 0x49454E44, 0xAE426082};

#else

static uint png_header[] = {
	1196314761, 169478669, 218103808, 1380206665, (((CAPTCHA_WIDTH << 24) & 0xff000000) | ((CAPTCHA_WIDTH << 8) & 0x00ff0000) | ((CAPTCHA_WIDTH >> 8) & 0x0000ff00) | ((CAPTCHA_WIDTH >> 24) & 0x000000ff)), (((CAPTCHA_HEIGHT << 24) & 0xff000000) | ((CAPTCHA_HEIGHT << 8) & 0x00ff0000) | ((CAPTCHA_HEIGHT >> 8) & 0x0000ff00) | ((CAPTCHA_HEIGHT >> 24) & 0x000000ff)), 520,
	3363454720U, 241, 1229091587, 134744148, 3763331547U, 0, 1413563465
};

static uint png_trailer[] = {0x00000000, 0x00000000, 0x444E4549, 0x826042AE};

#endif

static const uchar png_row_start[] = {0x00};

static bool png_output (uchar * d)
{
	memset(&zstrm, 0, sizeof(zstrm));
	
	if (deflateInit(&zstrm, 6) != Z_OK)
		return true;
	
	buf_expand(outputbuffer, sizeof(png_header));
	
	_BEGIN_LOCAL_SECTION_
	uint i, pos, sz;
	int r;
	uchar buf[CAPTCHA_ROWSIZE * 4];
	
	for (i = 0; i < CAPTCHA_HEIGHT; i++)
	{
		zstrm.next_out = buf;
		zstrm.avail_out = sizeof(buf);
		zstrm.next_in = (void *) png_row_start;
		zstrm.avail_in = sizeof(png_row_start);
		r = deflate(&zstrm, Z_NO_FLUSH);
		assert(r != Z_STREAM_ERROR);
		if (r == Z_STREAM_ERROR)
			return true;
		assert(zstrm.avail_out > 0);
		assert(zstrm.avail_in == 0);
		zstrm.next_in = d + i * CAPTCHA_ROWSIZE;
		zstrm.avail_in = CAPTCHA_ROWSIZE;
		r = deflate(&zstrm, Z_PARTIAL_FLUSH);
		assert(r != Z_STREAM_ERROR);
		assert(zstrm.avail_out > 0);
		assert(zstrm.avail_in == 0);
		sz = sizeof(buf) - zstrm.avail_out;
		pos = outputbuffer->cur_len;
		buf_expand(outputbuffer, sz);
		memcpy((uchar *) outputbuffer->data + pos, buf, sz);
	}
	
	zstrm.next_out = buf;
	zstrm.avail_out = sizeof(buf);
	r = deflate(&zstrm, Z_FINISH);
	assert(r == Z_STREAM_END);
	pos = outputbuffer->cur_len;
	buf_expand(outputbuffer, sizeof(buf) - zstrm.avail_out);
	memcpy((uchar *) outputbuffer->data + pos, buf, sizeof(buf) - zstrm.avail_out);
	png_header[12] = outputbuffer->cur_len - sizeof(png_header);
	#if __BYTE_ORDER == __LITTLE_ENDIAN
	png_header[12] = bswap_32(png_header[12]);
	#endif
	memcpy(outputbuffer->data, png_header, sizeof(png_header));
	* png_trailer = crc32(0, (uchar *) outputbuffer->data + (sizeof(png_header) - 4), outputbuffer->cur_len - (sizeof(png_header) - 4));
	#if __BYTE_ORDER == __LITTLE_ENDIAN
	* png_trailer = bswap_32(* png_trailer);
	#endif
	pos = outputbuffer->cur_len;
	buf_expand(outputbuffer, sizeof(png_trailer));
	memcpy((uchar *) outputbuffer->data + pos, png_trailer, sizeof(png_trailer));
	deflateEnd(&zstrm);
	_END_LOCAL_SECTION_
	
	return false;
}

#define CLAMP(x,l,u) (((x) < (l)) ? (l) : (((x) > (u)) ? (u) : (x)))
#define WITHIN(a, b, c) ((((a) <= (b)) && ((b) <= (c))) ? 1 : 0)

/* From GIMP sources (gimp_bilinear_pixels_8) */
static void bilinear_pixels (uchar * dest, double x, double y, uint bpp, bool has_alpha, uchar ** values)
{
	uint i;
	
	x = fmod(x, 1.0);
	y = fmod(y, 1.0);
	
	if (x < 0.0)
		x += 1.0;
	if (y < 0.0)
		y += 1.0;
	
	if (has_alpha)
	{
		uint ai = bpp - 1;
		double alpha0 = values[0][ai];
		double alpha1 = values[1][ai];
		double alpha2 = values[2][ai];
		double alpha3 = values[3][ai];
		double alpha = ((1.0 - y) * ((1.0 - x) * alpha0 + x * alpha1) + y * ((1.0 - x) * alpha2 + x * alpha3));
		
		dest[ai] = (uchar) alpha;
		
		if (dest[ai])
		{
			for (i = 0; i < ai; i++)
			{
				double m0 = ((1.0 - x) * values[0][i] * alpha0 + x * values[1][i] * alpha1);
				double m1 = ((1.0 - x) * values[2][i] * alpha2 + x * values[3][i] * alpha3);
				
				dest[i] = (uchar) (((1.0 - y) * m0 + y * m1) / alpha);
			}
		}
	}
	else
	{
		for (i = 0; i < bpp; i++)
		{
			double m0 = (1.0 - x) * values[0][i] + x * values[1][i];
			double m1 = (1.0 - x) * values[2][i] + x * values[3][i];
			
			dest[i] = (uchar) ((1.0 - y) * m0 + y * m1);
		}
	}
}

/* From GIMP sources (Waves plug-in) */
static void wave (uchar * src, uchar * dst, int width, int height, int bypp, bool has_alpha, double cen_x, double cen_y, double amplitude, double wavelength, double phase)
{
	long rowsiz;
	uchar * p;
	uchar * dest;
	int x1, y1, x2, y2;
	int x, y;
	int x1_in, y1_in, x2_in, y2_in;
	
	double xhsiz, yhsiz;
	double amnt, d;
	double needx, needy;
	double dx, dy;
	double xscale, yscale;
	
	int xi, yi;
	
	uchar * values[4];
	uchar zeroes[4] = { 0, 0, 0, 0 };
	
	phase = phase * M_PI / 180.0;
	rowsiz = width * bypp;
	
	x1 = y1 = 0;
	x2 = width;
	y2 = height;
	
	xhsiz = (double) (x2 - x1) / 2.0;
	yhsiz = (double) (y2 - y1) / 2.0;
	
	if (xhsiz < yhsiz)
	{
		xscale = yhsiz / xhsiz;
		yscale = 1.0;
	}
	else if (xhsiz > yhsiz)
	{
		xscale = 1.0;
		yscale = xhsiz / yhsiz;
	}
	else
	{
		xscale = 1.0;
		yscale = 1.0;
	}
	
	dst += y1 * rowsiz + x1 * bypp;
	
	wavelength *= 2;
	
	for (y = y1; y < y2; y++)
	{
		dest = dst;
		
		for (x = x1; x < x2; x++)
        {
			dx = (x - cen_x) * xscale;
			dy = (y - cen_y) * yscale;
			d = sqrt (dx * dx + dy * dy);
			
			amnt = amplitude * sin(((d / wavelength) * (2.0 * M_PI) + phase));
			needx = (amnt + dx) / xscale + cen_x;
			needy = (amnt + dy) / yscale + cen_y;
			
			xi = CLAMP(needx, 0, width - 2);
			yi = CLAMP(needy, 0, height - 2);
			
			p = src + rowsiz * yi + xi * bypp;
			
			x1_in = WITHIN(0, xi, width - 1);
			y1_in = WITHIN(0, yi, height - 1);
			x2_in = WITHIN(0, xi + 1, width - 1);
			y2_in = WITHIN(0, yi + 1, height - 1);
			
			if (x1_in && y1_in)
				values[0] = p;
			else
				values[0] = zeroes;
			
			if (x2_in && y1_in)
				values[1] = p + bypp;
			else
				values[1] = zeroes;
			
			if (x1_in && y2_in)
				values[2] = p + rowsiz;
			else
				values[2] = zeroes;
			
			if (x2_in && y2_in)
				values[3] = p + bypp + rowsiz;
			else
				values[3] = zeroes;
			
			bilinear_pixels(dest, needx, needy, bypp, has_alpha, values);
			dest += bypp;
		}
		
		dst += rowsiz;
	}
}

static void image_overlay (uchar * src, uint x, uint y, uint w, uint h, double angle)
{
	angle = angle * (M_PI / 180.0); /* Degrees to radians */
	
	_BEGIN_LOCAL_SECTION_
	double sa = sin(angle);
	double ca = cos(angle);
	double cen_x = (double) w / 2.0;
	double cen_y = (double) h / 2.0;
	
	uint i, k, rs = w * 3, offx = x * CAPTCHA_CHANNELS;
	uchar * s, * d;
	
	for (i = 0; i < h; i++)
		for (k = 0; k < w; k++)
		{
			s = src + i * rs + k * 3;
			if (s[0] == 0xFF && s[1] == 0xFF && s[2] == 0xFF)
				continue;
			d = blankimg[(lround(cen_y + (cen_x - (double) k) * sa + ca * ((double) i - cen_y + (cen_y - cen_x) * sa))) + y]
			+ offx + (lround((ca * ((double) k + cen_x * (ca - 1)) + sa * ((double) i + cen_y * (sa - 1))) / ca * ca + sa * sa)) * 3;
			d[0] = s[0];
			d[1] = s[1];
			d[2] = s[2];
		}
	_END_LOCAL_SECTION_
}

struct captcha_output * captcha_generate (struct captcha_output * output)
{
	if (syms == NULL)
		return NULL;
	
	buf_free(outputbuffer);
	memset(blankimg[0], 0xFF, CAPTCHA_ROWSIZE * CAPTCHA_HEIGHT);
	
	srand(current_time_msec);
	_BEGIN_LOCAL_SECTION_
	uchar nchars = (rand() % ((CAPTCHA_MAX_CHARS - CAPTCHA_MIN_CHARS) + 1)) + CAPTCHA_MIN_CHARS;
	uint st = (CAPTCHA_WIDTH - 2 * CAPTCHA_X_PADDING) / nchars;
	
	uint i;
	int pad;
	struct captcha_syms * s;
	uint x, y;
	
	for (i = 0; i < nchars; i++)
	{
		s = &(syms[rand() % syms_size]);
		pad = (int) ((int) st - (int) s->width);
		pad = max(1, pad);
		x = (i * st) + (rand() % pad) + CAPTCHA_X_PADDING;
		y = rand() % ((CAPTCHA_HEIGHT - 2 * CAPTCHA_Y_PADDING) - s->height) + CAPTCHA_Y_PADDING;
		image_overlay(s->rows, x, y, s->width, s->height, (rand() % (2 * CAPTCHA_ROTATE_ANGLE + 1)) - CAPTCHA_ROTATE_ANGLE);
		outputkeyword[i] = s->sym;
	}
	
	outputkeyword[i] = '\0';
	
	_BEGIN_LOCAL_SECTION_
	uchar * noise_pixel;
	
	for (i = 0; i < CAPTCHA_NOISE; i++)
	{
		noise_pixel = blankimg[rand() % CAPTCHA_HEIGHT] + (rand() % CAPTCHA_WIDTH) * CAPTCHA_CHANNELS;
		memset(noise_pixel, 0x00, CAPTCHA_CHANNELS);
	}
	
	wave(blankimg[0], outimg, CAPTCHA_WIDTH, CAPTCHA_HEIGHT, 3, false, (double) CAPTCHA_WIDTH / 2.0, (double) CAPTCHA_HEIGHT / 2.0, CAPTCHA_WAVES_AMPLITUDE, CAPTCHA_WAVES_LENGTH, (double) (rand() % 360));
	
	if (png_output(outimg))
		return NULL;
	
	output->keyword.str = outputkeyword;
	output->keyword.len = nchars;
	output->png_data.str = outputbuffer->data;
	output->png_data.len = outputbuffer->cur_len;
	_END_LOCAL_SECTION_
	_END_LOCAL_SECTION_
	
	return output;
}

struct ppm_image
{
	uint width;
	uint height;
	uchar * rows;
};

static void ppm_get_line (FILE * f, uchar * s, uint size)
{
	do
	{
		s = (uchar *) fgets((char *) s, size, f);
	}
	while (* s == '#');
}

static struct ppm_image * ppm_load (struct ppm_image * ppm, FILE * f)
{
	uchar buf[1024];
	uint width = 0, height = 0;
	ppm_get_line(f, buf, sizeof(buf));
	if (buf[0] != 'P' || buf[1] != '6')
		return NULL;
	ppm_get_line(f, buf, sizeof(buf));
	sscanf((const char *) buf, "%u %u", &width, &height);
	if (!(width && height))
		return NULL;
	ppm->width = width;
	ppm->height = height;
	ppm_get_line(f, buf, sizeof(buf));
	if (atoi((const char *) buf) != 255)
		return NULL;
	
	_BEGIN_LOCAL_SECTION_
	uchar * p = NULL;
	uint len = 0, r;
	
	while ((r = fread(buf, 1, sizeof(buf), f)) != 0)
	{
		len += r;
		p = realloc(p, len);
		memcpy(p + (len - r), buf, r);
	}
	
	if (len != width * height * 3)
		return NULL;
	
	ppm->rows = p;
	
	_END_LOCAL_SECTION_
	
	return ppm;
}

#ifdef _WIN
 #define glob(P, F, R, G) win32_glob(P, F, R, G)
 #define globfree(G) win32_globfree(G)
#endif

void captcha_init (void)
{
	pthread_mutex_lock(mutex);
	
	if (init == true)
		goto fn_end;
	
	init = true;
	_BEGIN_LOCAL_SECTION_
	char * captcha_pat = (char *) malloc(config.data.len + strlen(captcha_patterns) + 1);
	strcpy(captcha_pat, (char *) config.data.str);
	strcat(captcha_pat, captcha_patterns);
	
	_BEGIN_LOCAL_SECTION_
	glob_t globbuf;
	int ret = glob(captcha_pat, 0, NULL, &globbuf);
	
	if (ret != 0)
		peerr(0, "glob(%s)", captcha_pat);
	
	_BEGIN_LOCAL_SECTION_
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
		_BEGIN_LOCAL_SECTION_
		struct ppm_image ppm;
		if (ppm_load(&ppm, fp) == NULL)
			eerr(0, "Image corrupted: \"%s\"", globbuf.gl_pathv[i]);
		k = syms_size;
		syms_size++;
		syms = (struct captcha_syms *) realloc(syms, syms_size * sizeof(struct captcha_syms));
		syms[k].sym = TO_LOWER(* sym);
		syms[k].channels = 3;
		syms[k].rowbytes = ppm.width * syms[k].channels;
		syms[k].width = ppm.width;
		syms[k].height = ppm.height;
		syms[k].rows = ppm.rows;
		_END_LOCAL_SECTION_
		fclose(fp);
	}
	
	globfree(&globbuf);
	free(captcha_pat);
	
	_BEGIN_LOCAL_SECTION_
	uint * _crc = (uint *) (((uchar *) png_header) + 29);
	
	* _crc = crc32(0, ((uchar *) png_header) + 12, 17);
	#if __BYTE_ORDER == __LITTLE_ENDIAN
	* _crc = bswap_32(* _crc);
	#endif
	_END_LOCAL_SECTION_
	
	fn_end:
	
	if (blankimg == NULL)
	{
		blankimg = (uchar **) malloc(sizeof(uchar *) * CAPTCHA_HEIGHT);
		blankimg[0] = (uchar *) malloc(CAPTCHA_ROWSIZE * CAPTCHA_HEIGHT);
		outimg = (uchar *) malloc(CAPTCHA_ROWSIZE * CAPTCHA_HEIGHT);
		memset(outimg, 0xFF, CAPTCHA_ROWSIZE * CAPTCHA_HEIGHT);
		for (i = 0; i < CAPTCHA_HEIGHT; i++)
			blankimg[i] = (blankimg[0]) + i * CAPTCHA_ROWSIZE;
	}
	
	if (outputbuffer == NULL)
	{
		outputbuffer = buf_create(1, 256);
		outputkeyword = (uchar *) malloc(CAPTCHA_MAX_CHARS + 1);
		memset(outputkeyword, 0, CAPTCHA_MAX_CHARS + 1);
	}
	
	pthread_mutex_unlock(mutex);
	_END_LOCAL_SECTION_
	_END_LOCAL_SECTION_
	_END_LOCAL_SECTION_
}
