/*
 * Dcraw.hpp
 *
 *  Created on: 2016/06/19
 *      Author: kawai
 */

#ifndef DCRAW_HPP_
#define DCRAW_HPP_

namespace dcraw {

/*
   dcraw.c -- Dave Coffin's raw photo decoder
   Copyright 1997-2015 by Dave Coffin, dcoffin a cybercom o net

   This is a command-line ANSI C program to convert raw photos from
   any digital camera on any computer running any operating system.

   No license is required to download and use dcraw.c.  However,
   to lawfully redistribute dcraw, you must either (a) offer, at
   no extra charge, full source code* for all executable files
   containing RESTRICTED functions, (b) distribute this code under
   the GPL Version 2 or later, (c) remove all RESTRICTED functions,
   re-implement them, or copy them from an earlier, unrestricted
   Revision of dcraw.c, or (d) purchase a license from the author.

   The functions that process Foveon images have been RESTRICTED
   since Revision 1.237.  All other code remains free for all uses.

 *If you have not modified dcraw.c in any way, a link to my
   homepage qualifies as "full source code".

   $Revision: 1.476 $
   $Date: 2015/05/25 02:29:14 $
 */

#define DCRAW_VERSION "9.26"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _USE_MATH_DEFINES
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#if defined(DJGPP) || defined(__MINGW32__)
#define fseeko fseek
#define ftello ftell
#else
#define fgetc getc_unlocked
#endif
#ifdef __CYGWIN__
#include <io.h>
#endif
#ifdef WIN32
#include <sys/utime.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#define snprintf _snprintf
#define strcasecmp stricmp
#define strncasecmp strnicmp
typedef __int64 INT64;
typedef unsigned __int64 UINT64;
#else
#include <unistd.h>
#include <utime.h>
#include <netinet/in.h>
typedef long long INT64;
typedef unsigned long long UINT64;
#endif

#ifdef NODEPS
#define NO_JASPER
#define NO_JPEG
#define NO_LCMS
#endif
#ifndef NO_JASPER
#include <jasper/jasper.h>	/* Decode Red camera movies */
#endif
#ifndef NO_JPEG
#include <jpeglib.h>		/* Decode compressed Kodak DC120 photos */
#endif				/* and Adobe Lossy DNGs */
#ifndef NO_LCMS
#include <lcms2.h>		/* Support color profiles */
#endif
#ifdef LOCALEDIR
#include <libintl.h>
#define _(String) gettext(String)
#else
#define _(String) (String)
#endif

#if !defined(uchar)
#define uchar unsigned char
#endif
#if !defined(ushort)
#define ushort unsigned short
#endif

/*
   All global variables are defined here, and all functions that
   access them are prefixed with "CLASS".  Note that a thread-safe
   C++ class cannot have non-const static local variables.
 */
FILE *ifp, *ofp;
short order;
const char *ifname;
char *meta_data, xtrans[6][6], xtrans_abs[6][6];
char cdesc[5], desc[512], make[64], model[64], model2[64], artist[64];
float flash_used, canon_ev, iso_speed, shutter, aperture, focal_len;
time_t timestamp;
off_t strip_offset, data_offset;
off_t thumb_offset, meta_offset, profile_offset;
unsigned shot_order, kodak_cbpp, exif_cfa, unique_id;
unsigned thumb_length, meta_length, profile_length;
unsigned thumb_misc, *oprof, fuji_layout, shot_select=0, multi_out=0;
unsigned tiff_nifds, tiff_samples, tiff_bps, tiff_compress;
unsigned black, maximum, mix_green, raw_color, zero_is_bad;
unsigned zero_after_ff, is_raw, dng_version, is_foveon, data_error;
unsigned tile_width, tile_length, gpsdata[32], load_flags;
unsigned flip, tiff_flip, filters, colors;
ushort raw_height, raw_width, height, width, top_margin, left_margin;
ushort shrink, iheight, iwidth, fuji_width, thumb_width, thumb_height;
ushort *raw_image, (*image)[4], cblack[4102];
ushort white[8][8], curve[0x10000], cr2_slice[3], sraw_mul[4];
double pixel_aspect, aber[4]={1,1,1,1}, gamm[6]={ 0.45,4.5,0,0,0,0 };
float bright=1, user_mul[4]={0,0,0,0}, threshold=0;
int mask[8][4];
int half_size=0, four_color_rgb=0, document_mode=0, highlight=0;
int verbose=0, use_auto_wb=0, use_camera_wb=0, use_camera_matrix=1;
int output_color=1, output_bps=8, output_tiff=0, med_passes=0;
int no_auto_bright=0;
unsigned greybox[4] = { 0, 0, UINT_MAX, UINT_MAX };
float cam_mul[4], pre_mul[4], cmatrix[3][4], rgb_cam[3][4];
const double xyz_rgb[3][3] = {			/* XYZ from RGB */
		{ 0.412453, 0.357580, 0.180423 },
		{ 0.212671, 0.715160, 0.072169 },
		{ 0.019334, 0.119193, 0.950227 } };
const float d65_white[3] = { 0.950456, 1, 1.088754 };
int histogram[4][0x2000];
void (*write_thumb)(), (*write_fun)();
void (*load_raw)(), (*thumb_load_raw)();
jmp_buf failure;

struct decode {
	struct decode *branch[2];
	int leaf;
} first_decode[2048], *second_decode, *free_decode;

struct tiff_ifd {
	int width, height, bps, comp, phint, offset, flip, samples, bytes;
	int tile_width, tile_length;
	float shutter;
} tiff_ifd[10];

struct ph1 {
	int format, key_off, tag_21a;
	int black, split_col, black_col, split_row, black_row;
	float tag_210;
} ph1;

} // end of dcraw



#endif /* DCRAW_HPP_ */
