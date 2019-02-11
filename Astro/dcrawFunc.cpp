/*
 * dcrawFunc.cpp
 *
 *  Created on: Dec 22, 2018
 *      Author: kawai
 *
 *  20181222 NEFの読み込みに必要なコードに絞って、nefをグローバル変数から移動 メモリエラー解消したか？
 */

#include <math.h>
//#include <malloc/malloc.h>
#define DCRAW_VERSION "9.26"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _USE_MATH_DEFINES
extern "C" {
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
}

#if defined(DJGPP) || defined(__MINGW32__)
#define fseeko fseek
#define ftello ftell
#else
#define fgetc getc_unlocked
#endif
#ifdef __CYGWIN__
#include <io.h>
#endif
//#ifdef WIN32
#if defined(WIN32) || defined(OS)
#include <sys/utime.h>
#include <winsock2.h>
//#pragma comment(lib, "ws2_32.lib")
//#define snprintf _snprintf
#define strcasecmp stricmp
#define strncasecmp strnicmp
//typedef __int64 INT64;
//typedef unsigned __int64 UINT64;
#else
#include <unistd.h>
#include <utime.h>
#include <netinet/in.h>
typedef long long INT64;
typedef unsigned long long UINT64;
#endif

#define NODEPS

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
//#define uchar unsigned char
#endif
#if !defined(ushort)
//#define ushort unsigned short
#endif

#include "Utils++.hpp"
#include "dcrawFunc.hpp"

using namespace std;
//using namespace cv;
using namespace mycv;

namespace dcraw {


//MIN,MAXがOpenCVのマクロと重複するのでundef
#undef MIN
#undef MAX
//#define DBG_MESSAGE std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << "()" << std::endl;
//#define DBG_MESSAGE(message) std::cout << cv::format("[%5d]%s() : %s",__LINE__,__func__,message) << std::endl;
//#define DBG_MESSAGE(message) printf("[%5d]%s() : %s\n",__LINE__,__func__,message);
#define DBG_MESSAGE(message)
//#define DBG_MESSAGE(message) std::cout << cv::format("[%5d]%s() : %s", __LINE__, __func__, message) << endl;

#define DCRAW_VERSION "9.26"

//
//NEF readNEF0 (string fileName){
//	//	cout << "#" << __func__ << endl;
//	DBG_MESSAGE("before return(nef)");
//	int targc = 4;
//	const char **targv = (const char **)calloc(targc,sizeof(char *));
//	targv[0] = "dcraw::readNEF()";
//	targv[1] = "-T";
//	targv[2] = "-E";
//	targv[3] = fileName.c_str();
//	DBG_MESSAGE("before return(nef)");
//
//	dcraw::NEF nef = dcraw::readNEF00(targc, targv);
//	DBG_MESSAGE("before return(nef)");
//	free(targv);
//	return(nef);
//};


/*
   Identify which camera created this file, and set global variables
   accordingly.
 */
//使う情報だけ残す

#define CLASS

struct jhead {
	int algo, bits, high, wide, clrs, sraw, psv, restart, vpred[6];
	ushort quant[64], idct[64], *huff[20], *free[20], *row;
};

#ifndef __GLIBC__
char *my_memmem (char *haystack, size_t haystacklen,
		char *needle, size_t needlelen)
{
	char *c;
	for (c = haystack; c <= haystack + haystacklen - needlelen; c++)
		if (!memcmp (c, needle, needlelen))
			return c;
	return 0;
}
#define memmem my_memmem
char *my_strcasestr (char *haystack, const char *needle)
{
	char *c;
	for (c = haystack; *c; c++)
		if (!strncasecmp(c, needle, strlen(needle)))
			return c;
	return 0;
}
#define strcasestr my_strcasestr
#endif

void CLASS merror (void *ptr, const char *where)
{
	DBG_MESSAGE("Not implemented yet.");
	//	if (ptr) return;
	//	fprintf (stderr,_("%s: Out of memory in %s\n"), ifname, where);
	//	longjmp (failure, 1);
}

void CLASS derror()
{
	DBG_MESSAGE("Not implemented yet.");
	//	if (!data_error) {
	//		fprintf (stderr, "%s: ", ifname);
	//		if (feof(ifp))
	//			fprintf (stderr,_("Unexpected end of file\n"));
	//		else
	//			fprintf (stderr,_("Corrupt data near 0x%llx\n"), (INT64) ftello(ifp));
	//	}
	//	data_error++;
}

FILE *ifp;//, *ofp;
short order;
//const char *ifname;
unsigned is_raw;
unsigned dcraw_flip;

ushort CLASS sget2 (uchar *s)
{
	if (order == 0x4949)		/* "II" means little-endian */
		return s[0] | s[1] << 8;
	else				/* "MM" means big-endian */
		return s[0] << 8 | s[1];
}

ushort CLASS get2()
{
	uchar str[2] = { 0xff,0xff };
	fread (str, 1, 2, ifp);
	return sget2(str);
}

unsigned CLASS sget4 (uchar *s)
{
	if (order == 0x4949)
		return s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
	else
		return s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3];
}
#define sget4(s) sget4((uchar *)s)

unsigned CLASS get4()
{
	uchar str[4] = { 0xff,0xff,0xff,0xff };
	fread (str, 1, 4, ifp);
	return sget4(str);
}

unsigned CLASS getint (int type)
{
	return type == 3 ? get2() : get4();
}

float CLASS int_to_float (int i)
{
	union { int i; float f; } u;
	u.i = i;
	return u.f;
}

double CLASS getreal (int type)
{
	union { char c[8]; double d; } u;
	int i, rev;

	switch (type) {
	case 3: return (unsigned short) get2();
	case 4: return (unsigned int) get4();
	case 5:  u.d = (unsigned int) get4();
	return u.d / (unsigned int) get4();
	case 8: return (signed short) get2();
	case 9: return (signed int) get4();
	case 10: u.d = (signed int) get4();
	return u.d / (signed int) get4();
	case 11: return int_to_float (get4());
	case 12:
		rev = 7 * ((order == 0x4949) == (ntohs(0x1234) == 0x1234));
		for (i=0; i < 8; i++)
			u.c[i ^ rev] = fgetc(ifp);
		return u.d;
	default: return fgetc(ifp);
	}
}

void CLASS read_shorts (ushort *pixel, int count)
{
	DBG_MESSAGE("start");
	if (fread (pixel, 2, count, ifp) < count) derror();
	if ((order == 0x4949) == (ntohs(0x1234) == 0x1234))
		swab ((char *)pixel, (char *)pixel, count*2);
}

//データサイズが4byte以上でデータフィールドがデータポインタの場合、読出し位置をデータ先頭に移動している
//次のタグの位置がsaveに返される
void CLASS tiff_get (unsigned base,
		unsigned *tag, unsigned *type, unsigned *len, unsigned *save)
{
	*tag  = get2();
	*type = get2();
	*len  = get4();
	*save = ftell(ifp) + 4;
	if (*len * ("11124811248484"[*type < 14 ? *type:0]-'0') > 4)
		fseek (ifp, get4()+base, SEEK_SET);
}
/*
   Since the TIFF DateTime string has no timezone information,
   assume that the camera's clock was set to Universal Time.
 */
time_t CLASS get_timestamp (int reversed)
{
	time_t timestamp = 0;
	struct tm t;
	char str[20];
	int i;

	str[19] = 0;
	if (reversed)
		for (i=19; i--; ) str[i] = fgetc(ifp);
	else
		fread (str, 19, 1, ifp);
	memset (&t, 0, sizeof t);
	if (sscanf (str, "%d:%d:%d %d:%d:%d", &t.tm_year, &t.tm_mon,
			&t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) != 6) {
		cerr << ERROR_LINE << str << endl;
		exit(0);
	}

	t.tm_year -= 1900;
	t.tm_mon -= 1;
	t.tm_isdst = -1;
	if (mktime(&t) > 0)
		timestamp = mktime(&t);

	return(timestamp);
}

struct tiff_ifd {
	int width, height, bps, comp, phint, offset, flip, samples, bytes;
	int tile_width, tile_length;
	float shutter;
} tiff_ifd[10];
//TODO
unsigned tiff_nifds;
float flash_used, canon_ev, iso_speed, shutter, aperture, focal_len;
ushort raw_height, raw_width, height, width, top_margin, left_margin;
unsigned tiff_samples, tiff_bps, tiff_compress;
unsigned filters, colors;
unsigned shot_select = 0;
off_t data_offset, strip_offset, thumb_offset, meta_offset, profile_offset;
unsigned thumb_length, meta_length, profile_length;

unsigned tiff_flip, tile_width, tile_length;
// read at parse_tiff_ifd()
unsigned zero_after_ff, dng_version, is_foveon, data_error;
float cam_mul[4], pre_mul[4];
ushort cblack[4102];
char desc[512], make[64], model[64], model2[64], artist[64];
time_t timestamp;
unsigned black, maximum;
double pixel_aspect;
const float d65_white[3] = { 0.950456, 1, 1.088754 };
unsigned shot_order, kodak_cbpp, exif_cfa, unique_id;
unsigned gpsdata[32];
//void (*load_raw)(); //, (*thumb_load_raw)();
void (*load_raw)(NEF &nef); //, (*thumb_load_raw)();

#define FORC(cnt) for (c=0; c < cnt; c++)
#define FORC3 FORC(3)
#define FORC4 FORC(4)
#define FORCC FORC(colors)

#define SQR(x) ((x)*(x))
#define ABS(x) (((int)(x) ^ ((int)(x) >> 31)) - ((int)(x) >> 31))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define ULIM(x,y,z) ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y))
#define CLIP(x) LIM((int)(x),0,65535)
#define SWAP(a,b) { a=a+b; b=a-b; a=a-b; }

int CLASS ljpeg_start (struct jhead *jh, int info_only)
{
	ushort c, tag, len;
	uchar data[0x10000];
	const uchar *dp;

	memset (jh, 0, sizeof *jh);
	jh->restart = INT_MAX;
	if ((fgetc(ifp),fgetc(ifp)) != 0xd8) return 0;
	do {
		if (!fread (data, 2, 2, ifp)) return 0;
		tag =  data[0] << 8 | data[1];
		len = (data[2] << 8 | data[3]) - 2;
		if (tag <= 0xff00) return 0;
		fread (data, 1, len, ifp);
		switch (tag) {
		case 0xffc3:
			jh->sraw = ((data[7] >> 4) * (data[7] & 15) - 1) & 3;
		case 0xffc1:
		case 0xffc0:
			jh->algo = tag & 0xff;
			jh->bits = data[0];
			jh->high = data[1] << 8 | data[2];
			jh->wide = data[3] << 8 | data[4];
			jh->clrs = data[5] + jh->sraw;
			if (len == 9 && !dng_version) getc(ifp);
			break;
		case 0xffc4:
			if (info_only) break;
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			for (dp = data; dp < data+len && !((c = *dp++) & -20); )
			//				jh->free[c] = jh->huff[c] = make_decoder_ref (&dp);
			break;
		case 0xffda:
			jh->psv = data[1+data[0]*2];
			jh->bits -= data[3+data[0]*2] & 15;
			break;
		case 0xffdb:
			FORC(64) jh->quant[c] = data[c*2+1] << 8 | data[c*2+2];
			break;
		case 0xffdd:
			jh->restart = data[0] << 8 | data[1];
		}
	} while (tag != 0xffda);
	if (info_only) return 1;
	if (jh->clrs > 6 || !jh->huff[0]) return 0;
	FORC(19) if (!jh->huff[c+1]) jh->huff[c+1] = jh->huff[c];
	if (jh->sraw) {
		FORC(4)        jh->huff[2+c] = jh->huff[1];
		FORC(jh->sraw) jh->huff[1+c] = jh->huff[0];
	}
	cerr << ERROR_LINE << "before calloc" << endl; exit(0);
	jh->row = (ushort *) calloc (jh->wide*jh->clrs, 4);
	merror (jh->row, "ljpeg_start()");
	return zero_after_ff = 1;
}

void CLASS ljpeg_end (struct jhead *jh)
{
	int c;
	//FORC4 if (jh->free[c]) free (jh->free[c]);
	for (int c = 0; c < 4; c++) {
		if(jh->free[c]) {
			free (jh->free[c]);	jh->free[c] = NULL;
		}
	}
	free (jh->row); jh->row = NULL;
}

void CLASS parse_exif (int base);
void CLASS parse_gps (int base);
int CLASS parse_tiff (int base);
int CLASS parse_tiff_ifd (int base)
{
	unsigned entries, tag, type, len, plen=16, save;
	int ifd, use_cm=0, cfa, i, j, c, ima_len=0;
	char software[64], *cbuf, *cp;
	uchar cfa_pat[16], cfa_pc[] = { 0,1,2,3 }, tab[256];
	double cc[4][4], cm[4][3], cam_xyz[4][3], num;
	double ab[]={ 1,1,1,1 }, asn[] = { 0,0,0,0 }, xyz[] = { 1,1,1 };
	unsigned sony_curve[] = { 0,0,0,0,0,4095 };
	unsigned *buf, sony_offset=0, sony_length=0, sony_key=0;
	struct jhead jh;
	FILE *sfp;

	if (tiff_nifds >= sizeof tiff_ifd / sizeof tiff_ifd[0])
		return 1;
	ifd = tiff_nifds++;
	for (j=0; j < 4; j++)
		for (i=0; i < 4; i++)
			cc[j][i] = i == j;
	entries = get2();
	if (entries > 512) return 1;

	//unsigned entries0 = entries;
	//cout << __func__ << " " << entries << endl;
	while (entries--) {
		tiff_get (base, &tag, &type, &len, &save);
		//		cout << __func__ << " " << base << " " << tag << endl;

		//		ushort ttt = get2();
		//		cout << __func__ << cv::format("%3d %6u 0x%04x %8u %8d", entries, tag, tag, ttt, (short)(ttt)) << endl;
		switch (tag) {
		case 5:   width  = get2();  break;
		case 6:   height = get2();  break;
		case 7:   width += get2();  break;
		case 9:   if ((i = get2())) filters = i;  break;
		case 17: case 18:
			if (type == 3 && len == 1)
				cam_mul[(tag-17)*2] = get2() / 256.0;
			break;
		case 23:
			if (type == 3) iso_speed = get2();
			break;
		case 28: case 29: case 30:
			cblack[tag-28] = get2();
			cblack[3] = cblack[1];
			break;
		case 36: case 37: case 38:
			cam_mul[tag-36] = get2();
			break;
		case 39:
			if (len < 50 || cam_mul[0]) break;
			fseek (ifp, 12, SEEK_CUR);
			FORC3 cam_mul[c] = get2();
			break;
		case 46:
			cout << ERROR_LINE << "Not implemented yet." << endl;
			exit(0);
			//			if (type != 7 || fgetc(ifp) != 0xff || fgetc(ifp) != 0xd8) break;
			//			thumb_offset = ftell(ifp) - 2;
			//			thumb_length = len;
			break;
		case 61440:			/* Fuji HS10 table */
			fseek (ifp, get4()+base, SEEK_SET);
			parse_tiff_ifd (base);
			break;
		case 2: case 256: case 61441:	/* ImageWidth */
			tiff_ifd[ifd].width = getint(type);
			break;
		case 3: case 257: case 61442:	/* ImageHeight */
			tiff_ifd[ifd].height = getint(type);
			break;
		case 258:				/* BitsPerSample */
		case 61443:
			tiff_ifd[ifd].samples = len & 7;
			tiff_ifd[ifd].bps = getint(type);
			break;
		case 61446:
			cout << ERROR_LINE << "Not implemented yet." << endl;
			exit(0);
			//			raw_height = 0;
			//			if (tiff_ifd[ifd].bps > 12) break;
			//			load_raw = &CLASS packed_load_raw;
			//			load_flags = get4() ? 24:80;
			break;
		case 259:				/* Compression */
			tiff_ifd[ifd].comp = getint(type);
			break;
		case 262:				/* PhotometricInterpretation */
			tiff_ifd[ifd].phint = get2();
			break;
		case 270:				/* ImageDescription */
			fread (desc, 512, 1, ifp);
			break;
		case 271:				/* Make */
			fgets (make, 64, ifp);
			break;
		case 272:				/* Model */
			fgets (model, 64, ifp);
			break;
		case 280:				/* Panasonic RW2 offset */
			if (type != 4) break;
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			load_raw = &CLASS panasonic_load_raw;
			//			load_flags = 0x2008;
		case 273:				/* StripOffset */
		case 513:				/* JpegIFOffset */
		case 61447:
			tiff_ifd[ifd].offset = get4()+base;
			if (!tiff_ifd[ifd].bps && tiff_ifd[ifd].offset > 0) {
				fseek (ifp, tiff_ifd[ifd].offset, SEEK_SET);
				if (ljpeg_start (&jh, 1)) {
					tiff_ifd[ifd].comp    = 6;
					tiff_ifd[ifd].width   = jh.wide;
					tiff_ifd[ifd].height  = jh.high;
					tiff_ifd[ifd].bps     = jh.bits;
					tiff_ifd[ifd].samples = jh.clrs;
					if (!(jh.sraw || (jh.clrs & 1)))
						tiff_ifd[ifd].width *= jh.clrs;
					if ((tiff_ifd[ifd].width > 4*tiff_ifd[ifd].height) & ~jh.clrs) {
						tiff_ifd[ifd].width  /= 2;
						tiff_ifd[ifd].height *= 2;
					}
					i = order;
					parse_tiff (tiff_ifd[ifd].offset + 12);
					order = i;
				}
				ljpeg_end(&jh);
				//確保されていなさそう？
			}
			//cout << ERROR_LINE << "Not implemented yet. " << jh.row << endl;
			break;
		case 274:				/* Orientation */
			tiff_ifd[ifd].flip = "50132467"[get2() & 7]-'0';
			break;
		case 277:				/* SamplesPerPixel */
			tiff_ifd[ifd].samples = getint(type) & 7;
			break;
		case 279:				/* StripByteCounts */
		case 514:
		case 61448:
			tiff_ifd[ifd].bytes = get4();
			break;
		case 61454:
			FORC3 cam_mul[(4-c) % 3] = getint(type);
			break;
		case 305:  case 11:		/* Software */
			fgets (software, 64, ifp);
			if (!strncmp(software,"Adobe",5) ||
					!strncmp(software,"dcraw",5) ||
					!strncmp(software,"UFRaw",5) ||
					!strncmp(software,"Bibble",6) ||
					!strncmp(software,"Nikon Scan",10) ||
					!strcmp (software,"Digital Photo Professional"))
				is_raw = 0;
			break;
		case 306:				/* DateTime */
			timestamp = get_timestamp(0);
			break;
		case 315:				/* Artist */
			fread (artist, 64, 1, ifp);
			break;
		case 322:				/* TileWidth */
			tiff_ifd[ifd].tile_width = getint(type);
			break;
		case 323:				/* TileLength */
			tiff_ifd[ifd].tile_length = getint(type);
			break;
		case 324:				/* TileOffsets */
			tiff_ifd[ifd].offset = len > 1 ? ftell(ifp) : get4();
			if (len == 1)
				tiff_ifd[ifd].tile_width = tiff_ifd[ifd].tile_length = 0;
			if (len == 4) {
				cerr << ERROR_LINE << "error" << endl;
				exit(0);
				//				load_raw = &CLASS sinar_4shot_load_raw;
				//				is_raw = 5;
			}
			break;
		case 330:				/* SubIFDs */
			if (!strcmp(model,"DSLR-A100") && tiff_ifd[ifd].width == 3872) {
				cerr << ERROR_LINE << "error" << endl; exit(0);
				//				load_raw = &CLASS sony_arw_load_raw;
				//				data_offset = get4()+base;
				//				ifd++;  break;
			}
			while (len--) {
				i = ftell(ifp);
				fseek (ifp, get4()+base, SEEK_SET);
				if (parse_tiff_ifd (base)) break;
				fseek (ifp, i+4, SEEK_SET);
			}
			break;
		case 400:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			strcpy (make, "Sarnoff");
			//			maximum = 0xfff;
			break;
		case 28688:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			FORC4 sony_curve[c+1] = get2() >> 2 & 0xfff;
			//			for (i=0; i < 5; i++)
			//				for (j = sony_curve[i]+1; j <= sony_curve[i+1]; j++)
			//					curve[j] = curve[j-1] + (1 << i);
			break;
		case 29184: sony_offset = get4();  break;
		case 29185: sony_length = get4();  break;
		case 29217: sony_key    = get4();  break;
		case 29264:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			parse_minolta (ftell(ifp));
			//			raw_width = 0;
			break;
		case 29443:
			FORC4 cam_mul[c ^ (c < 2)] = get2();
			break;
		case 29459:
			FORC4 cam_mul[c] = get2();
			i = (cam_mul[1] == 1024 && cam_mul[2] == 1024) << 1;
			SWAP (cam_mul[i],cam_mul[i+1])
			break;
		case 33405:			/* Model2 */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			fgets (model2, 64, ifp);
			break;
		case 33421:			/* CFARepeatPatternDim */
			if (get2() == 6 && get2() == 6)
				filters = 9;
			break;
		case 33422:			/* CFAPattern */
			if (filters == 9) {
				cerr << ERROR_LINE << "error" << endl; exit(0);
				//FORC(36) ((char *)xtrans)[c] = fgetc(ifp) & 3;
			}
			break;
		case 64777:			/* Kodak P-series */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			if ((plen=len) > 16) plen = 16;
			//			fread (cfa_pat, 1, plen, ifp);
			//			for (colors=cfa=i=0; i < plen && colors < 4; i++) {
			//				colors += !(cfa & (1 << cfa_pat[i]));
			//				cfa |= 1 << cfa_pat[i];
			//			}
			//			if (cfa == 070) memcpy (cfa_pc,"\003\004\005",3);	/* CMY */
			//			if (cfa == 072) memcpy (cfa_pc,"\005\003\004\001",4);	/* GMCY */
			//			goto guess_cfa_pc;
		case 33424:
		case 65024:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			fseek (ifp, get4()+base, SEEK_SET);
			//			parse_kodak_ifd (base);
			break;
		case 33434:			/* ExposureTime */
			tiff_ifd[ifd].shutter = shutter = getreal(type);
			break;
		case 33437:			/* FNumber */
			aperture = getreal(type);
			break;
		case 34306:			/* Leaf white balance */
			FORC4 cam_mul[c ^ 1] = 4096.0 / get2();
			break;
		case 34307:			/* Leaf CatchLight color matrix */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			fread (software, 1, 7, ifp);
			//			if (strncmp(software,"MATRIX",6)) break;
			//			colors = 4;
			//			for (raw_color = i=0; i < 3; i++) {
			//				FORC4 fscanf (ifp, "%f", &rgb_cam[i][c^1]);
			//				if (!use_camera_wb) continue;
			//				num = 0;
			//				FORC4 num += rgb_cam[i][c];
			//				FORC4 rgb_cam[i][c] /= num;
			//			}
			break;
		case 34310:			/* Leaf metadata */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			parse_mos (ftell(ifp));
		case 34303:
			strcpy (make, "Leaf");
			break;
		case 34665:			/* EXIF tag */
			fseek (ifp, get4()+base, SEEK_SET);
			parse_exif (base);
			break;
		case 34853:			/* GPSInfo tag */
			fseek (ifp, get4()+base, SEEK_SET);
			parse_gps (base);
			break;
		case 34675:			/* InterColorProfile */
		case 50831:			/* AsShotICCProfile */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			profile_offset = ftell(ifp);
			//			profile_length = len;
			break;
		case 37122:			/* CompressedBitsPerPixel */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			kodak_cbpp = get4();
			break;
		case 37386:			/* FocalLength */
			focal_len = getreal(type);
			break;
		case 37393:			/* ImageNumber */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			shot_order = getint(type);
			break;
		case 37400:			/* old Kodak KDC tag */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			for (raw_color = i=0; i < 3; i++) {
			//				getreal(type);
			//				FORC3 rgb_cam[i][c] = getreal(type);
			//			}
			break;
		case 40976:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			strip_offset = get4();
			//			switch (tiff_ifd[ifd].comp) {
			//			case 32770: load_raw = &CLASS samsung_load_raw;   break;
			//			case 32772: load_raw = &CLASS samsung2_load_raw;  break;
			//			case 32773: load_raw = &CLASS samsung3_load_raw;  break;
			//			}
			break;
		case 46275:			/* Imacon tags */
			strcpy (make, "Imacon");
			data_offset = ftell(ifp);
			ima_len = len;
			break;
		case 46279:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			if (!ima_len) break;
			fseek (ifp, 38, SEEK_CUR);
			break;
		case 46274:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			fseek (ifp, 40, SEEK_CUR);
			//			raw_width  = get4();
			//			raw_height = get4();
			//			left_margin = get4() & 7;
			//			width = raw_width - left_margin - (get4() & 7);
			//			top_margin = get4() & 7;
			//			height = raw_height - top_margin - (get4() & 7);
			//			if (raw_width == 7262) {
			//				height = 5444;
			//				width  = 7244;
			//				left_margin = 7;
			//			}
			//			fseek (ifp, 52, SEEK_CUR);
			//			FORC3 cam_mul[c] = getreal(11);
			//			fseek (ifp, 114, SEEK_CUR);
			//			flip = (get2() >> 7) * 90;
			//			if (width * height * 6 == ima_len) {
			//				if (flip % 180 == 90) SWAP(width,height);
			//				raw_width = width;
			//				raw_height = height;
			//				left_margin = top_margin = filters = flip = 0;
			//			}
			//			sprintf (model, "Ixpress %d-Mp", height*width/1000000);
			//			load_raw = &CLASS imacon_full_load_raw;
			//			if (filters) {
			//				if (left_margin & 1) filters = 0x61616161;
			//				load_raw = &CLASS unpacked_load_raw;
			//			}
			//			maximum = 0xffff;
			break;
		case 50454:			/* Sinar tag */
		case 50455:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			if (!(cbuf = (char *) malloc(len))) break;
			//			fread (cbuf, 1, len, ifp);
			//			for (cp = cbuf-1; cp && cp < cbuf+len; cp = strchr(cp,'\n'))
			//				if (!strncmp (++cp,"Neutral ",8))
			//					sscanf (cp+8, "%f %f %f", cam_mul, cam_mul+1, cam_mul+2);
			//			free (cbuf);
			break;
		case 50458:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			if (!make[0]) strcpy (make, "Hasselblad");
			break;
		case 50459:			/* Hasselblad tag */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			i = order;
			//			j = ftell(ifp);
			//			c = tiff_nifds;
			//			order = get2();
			//			fseek (ifp, j+(get2(),get4()), SEEK_SET);
			//			parse_tiff_ifd (j);
			//			maximum = 0xffff;
			//			tiff_nifds = c;
			//			order = i;
			break;
		case 50706:			/* DNGVersion */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			FORC4 dng_version = (dng_version << 8) + fgetc(ifp);
			//			if (!make[0]) strcpy (make, "DNG");
			//			is_raw = 1;
			break;
		case 50708:			/* UniqueCameraModel */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			if (model[0]) break;
			//			fgets (make, 64, ifp);
			//			if ((cp = strchr(make,' '))) {
			//				strcpy(model,cp+1);
			//				*cp = 0;
			//			}
			break;
		case 50710:			/* CFAPlaneColor */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			if (filters == 9) break;
			//			if (len > 4) len = 4;
			//			colors = len;
			//			fread (cfa_pc, 1, colors, ifp);
			//			guess_cfa_pc:
			//			FORCC tab[cfa_pc[c]] = c;
			//			cdesc[c] = 0;
			//			for (i=16; i--; )
			//				filters = filters << 2 | tab[cfa_pat[i % plen]];
			//			filters -= !filters;
			break;
		case 50711:			/* CFALayout */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//if (get2() == 2) fuji_width = 1;
			break;
		case 291:
		case 50712:			/* LinearizationTable */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//linear_table (len);
			break;
		case 50713:			/* BlackLevelRepeatDim */
			cblack[4] = get2();
			cblack[5] = get2();
			if (cblack[4] * cblack[5] > sizeof cblack / sizeof *cblack - 6)
				cblack[4] = cblack[5] = 1;
			break;
		case 61450:
			cblack[4] = cblack[5] = MIN(sqrt(len),64);
			break;
		case 50714:			/* BlackLevel */
			if (!(cblack[4] * cblack[5]))
				cblack[4] = cblack[5] = 1;
			FORC (cblack[4] * cblack[5])
			cblack[6+c] = getreal(type);
			black = 0;
			break;
		case 50715:			/* BlackLevelDeltaH */
		case 50716:			/* BlackLevelDeltaV */
			for (num=i=0; i < len; i++)
				num += getreal(type);
			black += num/len + 0.5;
			break;
		case 50717:			/* WhiteLevel */
			maximum = getint(type);
			break;
		case 50718:			/* DefaultScale */
			pixel_aspect  = getreal(type);
			pixel_aspect /= getreal(type);
			break;
		case 50721:			/* ColorMatrix1 */
		case 50722:			/* ColorMatrix2 */
			FORCC for (j=0; j < 3; j++)
				cm[c][j] = getreal(type);
			use_cm = 1;
			break;
		case 50723:			/* CameraCalibration1 */
		case 50724:			/* CameraCalibration2 */
			for (i=0; i < colors; i++)
				FORCC cc[i][c] = getreal(type);
			break;
		case 50727:			/* AnalogBalance */
			FORCC ab[c] = getreal(type);
			break;
		case 50728:			/* AsShotNeutral */
			FORCC asn[c] = getreal(type);
			break;
		case 50729:			/* AsShotWhiteXY */
			xyz[0] = getreal(type);
			xyz[1] = getreal(type);
			xyz[2] = 1 - xyz[0] - xyz[1];
			FORC3 xyz[c] /= d65_white[c];
			break;
		case 50740:			/* DNGPrivateData */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			if (dng_version) break;
			//			parse_minolta (j = get4()+base);
			//			fseek (ifp, j, SEEK_SET);
			//			parse_tiff_ifd (base);
			break;
		case 50752:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//read_shorts (cr2_slice, 3);
			break;
		case 50829:			/* ActiveArea */
			top_margin = getint(type);
			left_margin = getint(type);
			height = getint(type) - top_margin;
			width = getint(type) - left_margin;
			break;
		case 50830:			/* MaskedAreas */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			for (i=0; i < len && i < 32; i++)
			//				((int *)mask)[i] = getint(type);
			//			black = 0;
			break;
		case 51009:			/* OpcodeList2 */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			meta_offset = ftell(ifp);
			break;
		case 64772:			/* Kodak P-series */
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			if (len < 13) break;
			//			fseek (ifp, 16, SEEK_CUR);
			//			data_offset = get4();
			//			fseek (ifp, 28, SEEK_CUR);
			//			data_offset += get4();
			//			load_raw = &CLASS packed_load_raw;
			break;
		case 65026:
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			if (type == 2) fgets (model2, 64, ifp);
		}
		fseek (ifp, save, SEEK_SET);
	}
	//cout << __func__ << " " << entries0 << " end" << endl;

	if (sony_length && (buf = (unsigned *) malloc(sony_length))) {
		cerr << ERROR_LINE << "error" << endl; exit(0);
		//
		//		fseek (ifp, sony_offset, SEEK_SET);
		//		fread (buf, sony_length, 1, ifp);
		//		sony_decrypt (buf, sony_length/4, 1, sony_key);
		//		sfp = ifp;
		//		if ((ifp = tmpfile())) {
		//			fwrite (buf, sony_length, 1, ifp);
		//			fseek (ifp, 0, SEEK_SET);
		//			parse_tiff_ifd (-sony_offset);
		//			fclose (ifp);
		//		}
		//		ifp = sfp;
		//		free (buf);
	}
	for (i=0; i < colors; i++)
		FORCC cc[i][c] *= ab[i];
	if (use_cm) {
		cerr << ERROR_LINE << "error" << endl; exit(0);

		//		FORCC for (i=0; i < 3; i++)
		//			for (cam_xyz[c][i]=j=0; j < colors; j++)
		//				cam_xyz[c][i] += cc[c][j] * cm[j][i] * xyz[i];
		//		cam_xyz_coeff (cmatrix, cam_xyz);
	}
	if (asn[0]) {
		cam_mul[3] = 0;
		FORCC cam_mul[c] = 1 / asn[c];
	}
	if (!use_cm) {
		FORCC pre_mul[c] /= cc[c][c];
	}
	return 0;
}

int CLASS parse_tiff (int base)
{
	int doff;

	fseek (ifp, base, SEEK_SET);
	order = get2();
	if (order != 0x4949 && order != 0x4d4d) return 0;
	get2(); //Z7はTIFF
	//cout << __func__ << " " << cv::format("0x%04x 2A:TIFF/2B:BigTiff", get2()) << endl;
	while ((doff = get4())) {
		//cout << __func__ << " " << base << " " << doff << endl;
		fseek (ifp, doff+base, SEEK_SET);
		if (parse_tiff_ifd (base)) break;
	}
	return 1;
}

void CLASS nikon_load_raw(NEF &nef);
void CLASS apply_tiff()
{
	int max_samp=0, ties=0, os, ns, raw=-1, thm=-1, i;
	struct jhead jh;

	//	thumb_misc = 16;
	//	if (thumb_offset) {
	//		fseek (ifp, thumb_offset, SEEK_SET);
	//		if (ljpeg_start (&jh, 1)) {
	//			thumb_misc   = jh.bits;
	//			thumb_width  = jh.wide;
	//			thumb_height = jh.high;
	//		}
	//	}

	for (i=tiff_nifds; i--; ) {
		if (tiff_ifd[i].shutter)
			shutter = tiff_ifd[i].shutter;
		tiff_ifd[i].shutter = shutter;
	}
	for (i=0; i < tiff_nifds; i++) {
		if (max_samp < tiff_ifd[i].samples)
			max_samp = tiff_ifd[i].samples;
		if (max_samp > 3) max_samp = 3;
		os = raw_width*raw_height;
		ns = tiff_ifd[i].width*tiff_ifd[i].height;
		if ((tiff_ifd[i].comp != 6 || tiff_ifd[i].samples != 3) &&
				(tiff_ifd[i].width | tiff_ifd[i].height) < 0x10000 &&
				ns && ((ns > os && (ties = 1)) ||
						(ns == os && shot_select == ties++))) {
			raw_width     = tiff_ifd[i].width;
			raw_height    = tiff_ifd[i].height;
			tiff_bps      = tiff_ifd[i].bps;
			tiff_compress = tiff_ifd[i].comp;
			data_offset   = tiff_ifd[i].offset;
			tiff_flip     = tiff_ifd[i].flip;
			tiff_samples  = tiff_ifd[i].samples;
			tile_width    = tiff_ifd[i].tile_width;
			tile_length   = tiff_ifd[i].tile_length;
			shutter       = tiff_ifd[i].shutter;
			raw = i;
		}
	}
	if (is_raw == 1 && ties) is_raw = ties;
	if (!tile_width ) tile_width  = INT_MAX;
	if (!tile_length) tile_length = INT_MAX;
	for (i=tiff_nifds; i--; )
		if (tiff_ifd[i].flip) tiff_flip = tiff_ifd[i].flip;
	if (raw >= 0 && !load_raw) {
		switch (tiff_compress) {
		//		case 32767:
		//			if (tiff_ifd[raw].bytes == raw_width*raw_height) {
		//				tiff_bps = 12;
		//				load_raw = &CLASS sony_arw2_load_raw;			break;
		//			}
		//			if (tiff_ifd[raw].bytes*8 != raw_width*raw_height*tiff_bps) {
		//				raw_height += 8;
		//				load_raw = &CLASS sony_arw_load_raw;			break;
		//			}
		//			load_flags = 79;
		//		case 32769:
		//			load_flags++;
		//		case 32770:
		//		case 32773: goto slr;
		//		case 0:  case 1:
		//			if (!strncmp(make,"OLYMPUS",7) &&
		//					tiff_ifd[raw].bytes*2 == raw_width*raw_height*3)
		//				load_flags = 24;
		//			if (tiff_ifd[raw].bytes*5 == raw_width*raw_height*8) {
		//				load_flags = 81;
		//				tiff_bps = 12;
		//			}
		//			slr:
		//
		//			switch (tiff_bps) {
		//			case  8: load_raw = &CLASS eight_bit_load_raw;	break;
		//			case 12: if (tiff_ifd[raw].phint == 2)
		//				load_flags = 6;
		//			load_raw = &CLASS packed_load_raw;		break;
		//			case 14: load_flags = 0;
		//			case 16: load_raw = &CLASS unpacked_load_raw;
		//			if (!strncmp(make,"OLYMPUS",7) &&
		//					tiff_ifd[raw].bytes*7 > raw_width*raw_height)
		//				load_raw = &CLASS olympus_load_raw;
		//			}
		//			break;
		//			case 6:  case 7:  case 99:
		//				load_raw = &CLASS lossless_jpeg_load_raw;		break;
		//			case 262:
		//				load_raw = &CLASS kodak_262_load_raw;			break;
		case 34713:
			load_raw = &CLASS nikon_load_raw; break;
			//				//				if ((raw_width+9)/10*16*raw_height == tiff_ifd[raw].bytes) {
			//				//					load_raw = &CLASS packed_load_raw;
			//				//					load_flags = 1;
			//				//				} else if (raw_width*raw_height*3 == tiff_ifd[raw].bytes*2) {
			//				//					load_raw = &CLASS packed_load_raw;
			//				//					if (model[0] == 'N') load_flags = 80;
			//				//				} else if (raw_width*raw_height*3 == tiff_ifd[raw].bytes) {
			//				//					load_raw = &CLASS nikon_yuv_load_raw;
			//				//					gamma_curve (1/2.4, 12.92, 1, 4095);
			//				//					memset (cblack, 0, sizeof cblack);
			//				//					filters = 0;
			//				//				} else if (raw_width*raw_height*2 == tiff_ifd[raw].bytes) {
			//				//					load_raw = &CLASS unpacked_load_raw;
			//				//					load_flags = 4;
			//				//					order = 0x4d4d;
			//				//				} else
			//				//					load_raw = &CLASS nikon_load_raw;			break;
			//				//			case 65535:
			//				//				load_raw = &CLASS pentax_load_raw;			break;
			//				//			case 65000:
			//				//				switch (tiff_ifd[raw].phint) {
			//				//				case 2: load_raw = &CLASS kodak_rgb_load_raw;   filters = 0;  break;
			//				//				case 6: load_raw = &CLASS kodak_ycbcr_load_raw; filters = 0;  break;
			//				//				case 32803: load_raw = &CLASS kodak_65000_load_raw;
			//				//				}
			//				//				case 32867: case 34892: break;
		default:
			cerr << ERROR_LINE << "error " << tiff_compress << endl;
			is_raw = 0;
			break;
		} //end of switch (tiff_compress)
	}

	/*	if (!dng_version)
		if ( (tiff_samples == 3 && tiff_ifd[raw].bytes && tiff_bps != 14 &&
				(tiff_compress & -16) != 32768)
				|| (tiff_bps == 8 && !strcasestr(make,"Kodak") &&
						!strstr(model2,"DEBUG RAW")))
			is_raw = 0;
	for (i=0; i < tiff_nifds; i++)
		if (i != raw && tiff_ifd[i].samples == max_samp &&
				tiff_ifd[i].width * tiff_ifd[i].height / (SQR(tiff_ifd[i].bps)+1) >
	thumb_width *       thumb_height / (SQR(thumb_misc)+1)
	&& tiff_ifd[i].comp != 34892) {
			thumb_width  = tiff_ifd[i].width;
			thumb_height = tiff_ifd[i].height;
			thumb_offset = tiff_ifd[i].offset;
			thumb_length = tiff_ifd[i].bytes;
			thumb_misc   = tiff_ifd[i].bps;
			thm = i;
		}
	if (thm >= 0) {
		thumb_misc |= tiff_ifd[thm].samples << 5;
		switch (tiff_ifd[thm].comp) {
		case 0:
			write_thumb = &CLASS layer_thumb;
			break;
		case 1:
			if (tiff_ifd[thm].bps <= 8)
				write_thumb = &CLASS ppm_thumb;
			else if (!strcmp(make,"Imacon"))
				write_thumb = &CLASS ppm16_thumb;
			else
				thumb_load_raw = &CLASS kodak_thumb_load_raw;
			break;
		case 65000:
			thumb_load_raw = tiff_ifd[thm].phint == 6 ?
					&CLASS kodak_ycbcr_load_raw : &CLASS kodak_rgb_load_raw;
		}
	}*/
}

unsigned CLASS getbithuff (int nbits, ushort *huff)
{
	static unsigned bitbuf=0;
	static int vbits=0, reset=0;
	unsigned c;

	if (nbits > 25) return 0;
	if (nbits < 0)
		return bitbuf = vbits = reset = 0;
	if (nbits == 0 || vbits < 0) return 0;
	while (!reset && vbits < nbits && (c = fgetc(ifp)) != EOF &&
			!(reset = zero_after_ff && c == 0xff && fgetc(ifp))) {
		bitbuf = (bitbuf << 8) + (uchar) c;
		vbits += 8;
	}
	c = bitbuf << (32-vbits) >> (32-nbits);
	if (huff) {
		vbits -= huff[c] >> 8;
		c = (uchar) huff[c];
	} else
		vbits -= nbits;
	if (vbits < 0) derror();
	return c;
}

#define getbits(n) getbithuff(n,0)
#define gethuff(h) getbithuff(*h,h+1)

/*
   Construct a decode tree according the specification in *source.
   The first 16 bytes specify how many codes should be 1-bit, 2-bit
   3-bit, etc.  Bytes after that are the leaf values.

   For example, if the source is

    { 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,
      0x04,0x03,0x05,0x06,0x02,0x07,0x01,0x08,0x09,0x00,0x0a,0x0b,0xff  },

   then the code is

	00		0x04
	010		0x03
	011		0x05
	100		0x06
	101		0x02
	1100		0x07
	1101		0x01
	11100		0x08
	11101		0x09
	11110		0x00
	111110		0x0a
	1111110		0x0b
	1111111		0xff
 */
ushort * CLASS make_decoder_ref (const uchar **source)
{
	int max, len, h, i, j;
	const uchar *count;
	ushort *huff = NULL;

	count = (*source += 16) - 17;
	for (max=16; max && !count[max]; max--);
	huff = (ushort *) calloc (1 + (1 << max), sizeof *huff);
	merror (huff, "make_decoder()");
	huff[0] = max;
	for (h=len=1; len <= max; len++)
		for (i=0; i < count[len]; i++, ++*source)
			for (j=0; j < 1 << (max-len); j++)
				if (h <= 1 << max)
					huff[h++] = len << 8 | **source;
	return huff;
}

ushort * CLASS make_decoder (const uchar *source)
{
	return make_decoder_ref (&source);
}

ushort curve[0x10000];
void CLASS nikon_load_raw(NEF &nef)
{
	DBG_MESSAGE("start");
	static const uchar nikon_tree[][32] = {
			{ 0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,5,4,3,6,2,7,1,0,8,9,11,10,12 }, /* 12-bit lossy */
			{ 0,1,5,1,1,1,1,1,1,2,0,0,0,0,0,0,0x39,0x5a,0x38,0x27,0x16,5,4,3,2,1,0,11,12,12 }, /* 12-bit lossy after split */
			{ 0,1,4,2,3,1,2,0,0,0,0,0,0,0,0,0,5,4,6,3,7,2,8,1,9,0,10,11,12 }, /* 12-bit lossless */
			{ 0,1,4,3,1,1,1,1,1,2,0,0,0,0,0,0,5,6,4,7,8,3,9,2,1,0,10,11,12,13,14 }, /* 14-bit lossy */
			{ 0,1,5,1,1,1,1,1,1,1,2,0,0,0,0,0,8,0x5c,0x4b,0x3a,0x29,7,6,5,4,3,2,1,0,13,14 }, /* 14-bit lossy after split */
			{ 0,1,4,2,2,3,1,2,0,0,0,0,0,0,0,0,7,6,8,5,9,4,10,3,11,12,2,0,1,13,14 }  /* 14-bit lossless */
	};
	ushort *huff, ver0, ver1, vpred[2][2], hpred[2], csize;
	int i, min, max, step=0, tree=0, split=0, row, col, len, shl, diff;
	huff = NULL;

	fseek (ifp, meta_offset, SEEK_SET);
	ver0 = fgetc(ifp);
	ver1 = fgetc(ifp);
	if (ver0 == 0x49 || ver1 == 0x58)
		fseek (ifp, 2110, SEEK_CUR);
	if (ver0 == 0x46) tree = 2;
	if (tiff_bps == 14) tree += 3;
	read_shorts (vpred[0], 4);
	max = 1 << tiff_bps & 0x7fff;
	if ((csize = get2()) > 1)
		step = max / (csize-1);
	if (ver0 == 0x44 && ver1 == 0x20 && step > 0) {
		for (i=0; i < csize; i++)
			curve[i*step] = get2();
		for (i=0; i < max; i++)
			curve[i] = ( curve[i-i%step]*(step-i%step) +
					curve[i-i%step+step]*(i%step) ) / step;
		fseek (ifp, meta_offset+562, SEEK_SET);
		split = get2();
	} else if (ver0 != 0x46 && csize <= 0x4001)
		read_shorts (curve, max=csize);
	while (curve[max-2] == curve[max-1]) max--;
	huff = make_decoder (nikon_tree[tree]); //	huff = (ushort *) calloc (1 + (1 << max), sizeof *huff);
	fseek (ifp, data_offset, SEEK_SET);
	getbits(-1);
	DBG_MESSAGE("for()");

	nef.width = raw_width;
	nef.height = height;
	DBG_MESSAGE("before");
	//	nef.bayer = Mat_<unsigned short>(nef.height, nef.width);
	nef.bayer = cv::Mat::zeros(nef.height, nef.width, CV_16UC1);
	//べつのところでとまるようになった...
	DBG_MESSAGE("after");

	for (min=row=0; row < height; row++) {
		if (split && row == split) {
			cout << __LINE__ << " free(huff)" << endl;
			free (huff); huff = NULL;
			huff = make_decoder (nikon_tree[tree+1]);
			max += (min = 16) << 1;
		}
		for (col=0; col < raw_width; col++) {
			i = gethuff(huff);
			len = i & 15;
			shl = i >> 4;
			diff = ((getbits(len-shl) << 1) + 1) << shl >> 1;
			if ((diff & (1 << (len-1))) == 0)
				diff -= (1 << len) - !shl;
			if (col < 2) hpred[col] = vpred[row & 1][col] += diff;
			else	   hpred[col & 1] += diff;
			if ((ushort)(hpred[col & 1] + min) >= max) derror();

			//RAW(row,col) = curve[LIM((short)hpred[col & 1],0,0x3fff)];
			nef.bayer(row, col) = curve[LIM((short)hpred[col & 1],0,0x3fff)];
		}
	}
	if(huff != NULL) {
		//cout << __LINE__ << " free(huff)" << endl;
		free (huff); huff = NULL;
		//assert( malloc_zone_check(NULL) );
	}
	DBG_MESSAGE("end");
	//	imwrite("I0.tif",I0);
	return;
}

void CLASS parse_makernote (int base, int uptag)
{
	static const uchar xlat[2][256] = {
			{ 0xc1,0xbf,0x6d,0x0d,0x59,0xc5,0x13,0x9d,0x83,0x61,0x6b,0x4f,0xc7,0x7f,0x3d,0x3d,
					0x53,0x59,0xe3,0xc7,0xe9,0x2f,0x95,0xa7,0x95,0x1f,0xdf,0x7f,0x2b,0x29,0xc7,0x0d,
					0xdf,0x07,0xef,0x71,0x89,0x3d,0x13,0x3d,0x3b,0x13,0xfb,0x0d,0x89,0xc1,0x65,0x1f,
					0xb3,0x0d,0x6b,0x29,0xe3,0xfb,0xef,0xa3,0x6b,0x47,0x7f,0x95,0x35,0xa7,0x47,0x4f,
					0xc7,0xf1,0x59,0x95,0x35,0x11,0x29,0x61,0xf1,0x3d,0xb3,0x2b,0x0d,0x43,0x89,0xc1,
					0x9d,0x9d,0x89,0x65,0xf1,0xe9,0xdf,0xbf,0x3d,0x7f,0x53,0x97,0xe5,0xe9,0x95,0x17,
					0x1d,0x3d,0x8b,0xfb,0xc7,0xe3,0x67,0xa7,0x07,0xf1,0x71,0xa7,0x53,0xb5,0x29,0x89,
					0xe5,0x2b,0xa7,0x17,0x29,0xe9,0x4f,0xc5,0x65,0x6d,0x6b,0xef,0x0d,0x89,0x49,0x2f,
					0xb3,0x43,0x53,0x65,0x1d,0x49,0xa3,0x13,0x89,0x59,0xef,0x6b,0xef,0x65,0x1d,0x0b,
					0x59,0x13,0xe3,0x4f,0x9d,0xb3,0x29,0x43,0x2b,0x07,0x1d,0x95,0x59,0x59,0x47,0xfb,
					0xe5,0xe9,0x61,0x47,0x2f,0x35,0x7f,0x17,0x7f,0xef,0x7f,0x95,0x95,0x71,0xd3,0xa3,
					0x0b,0x71,0xa3,0xad,0x0b,0x3b,0xb5,0xfb,0xa3,0xbf,0x4f,0x83,0x1d,0xad,0xe9,0x2f,
					0x71,0x65,0xa3,0xe5,0x07,0x35,0x3d,0x0d,0xb5,0xe9,0xe5,0x47,0x3b,0x9d,0xef,0x35,
					0xa3,0xbf,0xb3,0xdf,0x53,0xd3,0x97,0x53,0x49,0x71,0x07,0x35,0x61,0x71,0x2f,0x43,
					0x2f,0x11,0xdf,0x17,0x97,0xfb,0x95,0x3b,0x7f,0x6b,0xd3,0x25,0xbf,0xad,0xc7,0xc5,
					0xc5,0xb5,0x8b,0xef,0x2f,0xd3,0x07,0x6b,0x25,0x49,0x95,0x25,0x49,0x6d,0x71,0xc7 },
					{ 0xa7,0xbc,0xc9,0xad,0x91,0xdf,0x85,0xe5,0xd4,0x78,0xd5,0x17,0x46,0x7c,0x29,0x4c,
							0x4d,0x03,0xe9,0x25,0x68,0x11,0x86,0xb3,0xbd,0xf7,0x6f,0x61,0x22,0xa2,0x26,0x34,
							0x2a,0xbe,0x1e,0x46,0x14,0x68,0x9d,0x44,0x18,0xc2,0x40,0xf4,0x7e,0x5f,0x1b,0xad,
							0x0b,0x94,0xb6,0x67,0xb4,0x0b,0xe1,0xea,0x95,0x9c,0x66,0xdc,0xe7,0x5d,0x6c,0x05,
							0xda,0xd5,0xdf,0x7a,0xef,0xf6,0xdb,0x1f,0x82,0x4c,0xc0,0x68,0x47,0xa1,0xbd,0xee,
							0x39,0x50,0x56,0x4a,0xdd,0xdf,0xa5,0xf8,0xc6,0xda,0xca,0x90,0xca,0x01,0x42,0x9d,
							0x8b,0x0c,0x73,0x43,0x75,0x05,0x94,0xde,0x24,0xb3,0x80,0x34,0xe5,0x2c,0xdc,0x9b,
							0x3f,0xca,0x33,0x45,0xd0,0xdb,0x5f,0xf5,0x52,0xc3,0x21,0xda,0xe2,0x22,0x72,0x6b,
							0x3e,0xd0,0x5b,0xa8,0x87,0x8c,0x06,0x5d,0x0f,0xdd,0x09,0x19,0x93,0xd0,0xb9,0xfc,
							0x8b,0x0f,0x84,0x60,0x33,0x1c,0x9b,0x45,0xf1,0xf0,0xa3,0x94,0x3a,0x12,0x77,0x33,
							0x4d,0x44,0x78,0x28,0x3c,0x9e,0xfd,0x65,0x57,0x16,0x94,0x6b,0xfb,0x59,0xd0,0xc8,
							0x22,0x36,0xdb,0xd2,0x63,0x98,0x43,0xa1,0x04,0x87,0x86,0xf7,0xa6,0x26,0xbb,0xd6,
							0x59,0x4d,0xbf,0x6a,0x2e,0xaa,0x2b,0xef,0xe6,0x78,0xb6,0x4e,0xe0,0x2f,0xdc,0x7c,
							0xbe,0x57,0x19,0x32,0x7e,0x2a,0xd0,0xb8,0xba,0x29,0x00,0x3c,0x52,0x7d,0xa8,0x49,
							0x3b,0x2d,0xeb,0x25,0x49,0xfa,0xa3,0xaa,0x39,0xa7,0xc5,0xa7,0x50,0x11,0x36,0xfb,
							0xc6,0x67,0x4a,0xf5,0xa5,0x12,0x65,0x7e,0xb0,0xdf,0xaf,0x4e,0xb3,0x61,0x7f,0x2f } };
	unsigned offset=0, entries, tag, type, len, save, c, entries0;
	unsigned ver97=0, serial=0, i, wbi=0, wb[4]={0,0,0,0};
	uchar buf97[324], ci, cj, ck;
	short morder, sorder=order;
	char buf[10];
	/*
   The MakerNote might have its own TIFF header (possibly with
   its own byte-order!), or it might just be a table.
	 */
	if (!strcmp(make,"Nokia")) return;
	fread (buf, 1, 10, ifp);
	if (!strncmp (buf,"KDK" ,3) ||	/* these aren't TIFF tables */
			!strncmp (buf,"VER" ,3) ||
			!strncmp (buf,"IIII",4) ||
			!strncmp (buf,"MMMM",4)) return;
	if (!strncmp (buf,"KC"  ,2) ||	/* Konica KD-400Z, KD-510Z */
			!strncmp (buf,"MLY" ,3)) {	/* Minolta DiMAGE G series */
		order = 0x4d4d;
		while ((i=ftell(ifp)) < data_offset && i < 16384) {
			wb[0] = wb[2];  wb[2] = wb[1];  wb[1] = wb[3];
			wb[3] = get2();
			if (wb[1] == 256 && wb[3] == 256 &&
					wb[0] > 256 && wb[0] < 640 && wb[2] > 256 && wb[2] < 640)
				FORC4 cam_mul[c] = wb[c];
		}
		goto quit;
	}
	if (!strcmp (buf,"Nikon")) {
		base = ftell(ifp);
		order = get2();
		if (get2() != 42) goto quit;
		offset = get4();
		fseek (ifp, offset-8, SEEK_CUR);
	} else if (!strcmp (buf,"OLYMPUS") ||
			!strcmp (buf,"PENTAX ")) {
		base = ftell(ifp)-10;
		fseek (ifp, -2, SEEK_CUR);
		order = get2();
		if (buf[0] == 'O') get2();
	} else if (!strncmp (buf,"SONY",4) ||
			!strcmp  (buf,"Panasonic")) {
		goto nf;
	} else if (!strncmp (buf,"FUJIFILM",8)) {
		base = ftell(ifp)-10;
		nf: order = 0x4949;
		fseek (ifp,  2, SEEK_CUR);
	} else if (!strcmp (buf,"OLYMP") ||
			!strcmp (buf,"LEICA") ||
			!strcmp (buf,"Ricoh") ||
			!strcmp (buf,"EPSON"))
		fseek (ifp, -2, SEEK_CUR);
	else if (!strcmp (buf,"AOC") ||
			!strcmp (buf,"QVC"))
		fseek (ifp, -4, SEEK_CUR);
	else {
		fseek (ifp, -10, SEEK_CUR);
		if (!strncmp(make,"SAMSUNG",7))
			base = ftell(ifp);
	}
	entries = get2();
	if (entries > 1000) return;
	morder = order;
	//	entries0 = entries;
	//	cout << __func__ << " " << entries << endl;
	while (entries--) {
		order = morder;
		tiff_get (base, &tag, &type, &len, &save);
		tag |= uptag << 16;

		//		unsigned long ttt = ftell(ifp);
		//		unsigned char d4[4];
		//		fread(d4, 1, 4, ifp);
		//		cout << cv::format("\t\t0x%04x %2d %8d %02x %02x %02x %02x %16d",
		//				tag, type, len, d4[0], d4[1], d4[2], d4[3], get4(d4)) << endl;
		////				(((((d4[0] << 8) + d4[1]) << 8) + d4[2]) << 8) + d4[3]) << endl;
		//		fseek(ifp, ttt, SEEK_SET);

		//cout << "\t" << entries << " "<< tag << " " << type << " " << len << " " << save << endl;
		if (tag == 2 && strstr(make,"NIKON") && !iso_speed)
			iso_speed = (get2(),get2());
		if (tag == 4 && len > 26 && len < 35) {
			if ((i=(get4(),get2())) != 0x7fff && !iso_speed)
				iso_speed = 50 * pow (2, i/32.0 - 4);
			if ((i=(get2(),get2())) != 0x7fff && !aperture)
				aperture = pow (2, i/64.0);
			if ((i=get2()) != 0xffff && !shutter)
				shutter = pow (2, (short) i/-32.0);
			wbi = (get2(),get2());
			shot_order = (get2(),get2());
		}
		if ((tag == 4 || tag == 0x114) && !strncmp(make,"KONICA",6)) {
			fseek (ifp, tag == 4 ? 140:160, SEEK_CUR);
			switch (get2()) {
			case 72:  dcraw_flip = 0;  break;
			case 76:  dcraw_flip = 6;  break;
			case 82:  dcraw_flip = 5;  break;
			}
		}
		if (tag == 7 && type == 2 && len > 20)
			fgets (model2, 64, ifp);
		if (tag == 8 && type == 4)
			shot_order = get4();
		if (tag == 9 && !strcmp(make,"Canon"))
			fread (artist, 64, 1, ifp);
		if (tag == 0xc && len == 4)
			FORC3 cam_mul[(c << 1 | c >> 1) & 3] = getreal(type);
		if (tag == 0xd && type == 7 && get2() == 0xaaaa) {
			for (c=i=2; (ushort) c != 0xbbbb && i < len; i++)
				c = c << 8 | fgetc(ifp);
			while ((i+=4) < len-5)
				if (get4() == 257 && (i=len) && (c = (get4(),fgetc(ifp))) < 3)
					dcraw_flip = "065"[c]-'0';
		}
		if (tag == 0x10 && type == 4)
			unique_id = get4();
		if (tag == 0x11 && is_raw && !strncmp(make,"NIKON",5)) {
			fseek (ifp, get4()+base, SEEK_SET);
			parse_tiff_ifd (base);
		}
		if (tag == 0x14 && type == 7) {
			if (len == 2560) {
				fseek (ifp, 1248, SEEK_CUR);
				goto get2_256;
			}
			fread (buf, 1, 10, ifp);
			if (!strncmp(buf,"NRW ",4)) {
				fseek (ifp, strcmp(buf+4,"0100") ? 46:1546, SEEK_CUR);
				cam_mul[0] = get4() << 2;
				cam_mul[1] = get4() + get4();
				cam_mul[2] = get4() << 2;
			}
		}
		if (tag == 0x15 && type == 2 && is_raw)
			fread (model, 64, 1, ifp);
		if (strstr(make,"PENTAX")) {
			if (tag == 0x1b) tag = 0x1018;
			if (tag == 0x1c) tag = 0x1017;
		}
		if (tag == 0x1d)
			while ((c = fgetc(ifp)) && c != EOF)
				serial = serial*10 + (isdigit(c) ? c - '0' : c % 10);
		if (tag == 0x29 && type == 1) {
			c = wbi < 18 ? "012347800000005896"[wbi]-'0' : 0;
			fseek (ifp, 8 + c*32, SEEK_CUR);
			FORC4 cam_mul[c ^ (c >> 1) ^ 1] = get4();
		}
		if (tag == 0x3d && type == 3 && len == 4)
			FORC4 cblack[c ^ c >> 1] = get2() >> (14-tiff_ifd[2].bps);
		if (tag == 0x81 && type == 4) {
			data_offset = get4();
			fseek (ifp, data_offset + 41, SEEK_SET);
			raw_height = get2() * 2;
			raw_width  = get2();
			filters = 0x61616161;
		}
		if ((tag == 0x81  && type == 7) ||
				(tag == 0x100 && type == 7) ||
				(tag == 0x280 && type == 1)) {
			thumb_offset = ftell(ifp);
			thumb_length = len;
		}
		if (tag == 0x88 && type == 4 && (thumb_offset = get4()))
			thumb_offset += base;
		if (tag == 0x89 && type == 4)
			thumb_length = get4();
		if (tag == 0x8c || tag == 0x96)
			meta_offset = ftell(ifp);
		if (tag == 0x91) {
			//cerr << ERROR_LINE << "error" << endl; exit(0);
			//parse_debuginfo(base, len); // add by me
		}
		if (tag == 0x97) {
			for (i=0; i < 4; i++)
				ver97 = ver97 * 10 + fgetc(ifp)-'0';
			switch (ver97) {
			case 100:
				fseek (ifp, 68, SEEK_CUR);
				FORC4 cam_mul[(c >> 1) | ((c & 1) << 1)] = get2();
				break;
			case 102:
				fseek (ifp, 6, SEEK_CUR);
				FORC4 cam_mul[c ^ (c >> 1)] = get2();
				break;
			case 103:
				fseek (ifp, 16, SEEK_CUR);
				FORC4 cam_mul[c] = get2();
			}
			if (ver97 >= 200) {
				if (ver97 != 205) fseek (ifp, 280, SEEK_CUR);
				fread (buf97, 324, 1, ifp);
			}
		}
		if (tag == 0xa1 && type == 7) {
			order = 0x4949;
			fseek (ifp, 140, SEEK_CUR);
			FORC3 cam_mul[c] = get4();
		}
		if (tag == 0xa4 && type == 3) {
			fseek (ifp, wbi*48, SEEK_CUR);
			FORC3 cam_mul[c] = get2();
		}
		if (tag == 0xa7 && (unsigned) (ver97-200) < 17) {
			ci = xlat[0][serial & 0xff];
			cj = xlat[1][fgetc(ifp)^fgetc(ifp)^fgetc(ifp)^fgetc(ifp)];
			ck = 0x60;
			for (i=0; i < 324; i++)
				buf97[i] ^= (cj += ci * ck++);
			i = "66666>666;6A;:;55"[ver97-200] - '0';
			FORC4 cam_mul[c ^ (c >> 1) ^ (i & 1)] =
					sget2 (buf97 + (i & -2) + c*2);
		}
		if (tag == 0x200 && len == 3)
			shot_order = (get4(),get4());
		if (tag == 0x200 && len == 4)
			FORC4 cblack[c ^ c >> 1] = get2();
		if (tag == 0x201 && len == 4)
			FORC4 cam_mul[c ^ (c >> 1)] = get2();
		if (tag == 0x220 && type == 7)
			meta_offset = ftell(ifp);
		if (tag == 0x401 && type == 4 && len == 4)
			FORC4 cblack[c ^ c >> 1] = get4();
		if (tag == 0xe01) {		/* Nikon Capture Note */
			order = 0x4949;
			fseek (ifp, 22, SEEK_CUR);
			for (offset=22; offset+22 < len; offset += 22+i) {
				tag = get4();
				fseek (ifp, 14, SEEK_CUR);
				i = get4()-4;
				if (tag == 0x76a43207) dcraw_flip = get2();
				else fseek (ifp, i, SEEK_CUR);
			}
		}
		if (tag == 0xe80 && len == 256 && type == 7) {
			fseek (ifp, 48, SEEK_CUR);
			cam_mul[0] = get2() * 508 * 1.078 / 0x10000;
			cam_mul[2] = get2() * 382 * 1.173 / 0x10000;
		}
		if (tag == 0xf00 && type == 7) {
			if (len == 614)
				fseek (ifp, 176, SEEK_CUR);
			else if (len == 734 || len == 1502)
				fseek (ifp, 148, SEEK_CUR);
			else goto next;
			goto get2_256;
		}
		if ((tag == 0x1011 && len == 9) || tag == 0x20400200) {
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			for (i=0; i < 3; i++) {
			//				FORC3 cmatrix[i][c] = ((short) get2()) / 256.0;
			//			}
		}
		if ((tag == 0x1012 || tag == 0x20400600) && len == 4)
			FORC4 cblack[c ^ c >> 1] = get2();
		if (tag == 0x1017 || tag == 0x20400100)
			cam_mul[0] = get2() / 256.0;
		if (tag == 0x1018 || tag == 0x20400100)
			cam_mul[2] = get2() / 256.0;
		if (tag == 0x2011 && len == 2) {
			get2_256:
			order = 0x4d4d;
			cam_mul[0] = get2() / 256.0;
			cam_mul[2] = get2() / 256.0;
		}
		if ((tag | 0x70) == 0x2070 && (type == 4 || type == 13))
			fseek (ifp, get4()+base, SEEK_SET);
		if (tag == 0x2020) {
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//parse_thumb_note (base, 257, 258);
		}
		if (tag == 0x2040)
			parse_makernote (base, 0x2040);
		if (tag == 0xb028) {
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			fseek (ifp, get4()+base, SEEK_SET);
			//			parse_thumb_note (base, 136, 137);
		}
		if (tag == 0x4001 && len > 500) {
			cerr << ERROR_LINE << "error" << endl; exit(0);
			//			i = len == 582 ? 50 : len == 653 ? 68 : len == 5120 ? 142 : 126;
			//			fseek (ifp, i, SEEK_CUR);
			//			FORC4 cam_mul[c ^ (c >> 1)] = get2();
			//			for (i+=18; i <= len; i+=10) {
			//				get2();
			//				FORC4 sraw_mul[c ^ (c >> 1)] = get2();
			//				if (sraw_mul[1] == 1170) break;
			//			}
		}
		if (tag == 0x4021 && get4() && get4())
			FORC4 cam_mul[c] = 1024;
		if (tag == 0xa021)
			FORC4 cam_mul[c ^ (c >> 1)] = get4();
		if (tag == 0xa028)
			FORC4 cam_mul[c ^ (c >> 1)] -= get4();
		if (tag == 0xb001)
			unique_id = get2();
		next:
		fseek (ifp, save, SEEK_SET);
	}
	quit:
	order = sorder;
	//cout << __func__ << " " << entries0 << " end" << endl;
}

void CLASS parse_exif (int base)
{
	unsigned kodak, entries, tag, type, len, save, c;
	double expo;

	kodak = !strncmp(make,"EASTMAN",7) && tiff_nifds < 3;
	entries = get2();
	//	unsigned int entries0 = entries;
	//	cout << __func__ << " " << entries << endl;
	while (entries--) {
		tiff_get (base, &tag, &type, &len, &save);
		//cout << "\t" << entries << " "<< tag << " " << type << " " << len << endl;
		switch (tag) {
		case 33434:  tiff_ifd[tiff_nifds-1].shutter =
				shutter = getreal(type);		break;
		case 33437:  aperture = getreal(type);		break;
		case 34855:  iso_speed = get2();			break;
		case 36867:
		case 36868:  get_timestamp(0);			break;
		case 37377:  if ((expo = -getreal(type)) < 128)
			tiff_ifd[tiff_nifds-1].shutter =
					shutter = pow (2, expo);		break;
		case 37378:  aperture = pow (2, getreal(type)/2);	break;
		case 37386:  focal_len = getreal(type);		break;
		case 37500:  parse_makernote (base, 0);		break;
		case 40962:  if (kodak) raw_width  = get4();	break;
		case 40963:  if (kodak) raw_height = get4();	break;
		case 41730:
			if (get4() == 0x20002)
				for (exif_cfa=c=0; c < 8; c+=2)
					exif_cfa |= fgetc(ifp) * 0x01010101 << c;
		}
		fseek (ifp, save, SEEK_SET);
	}
	//	cout << __func__ << " " << entries0 << " end" << endl;
}

void CLASS parse_gps (int base)
{
	unsigned entries, tag, type, len, save, c;

	entries = get2();
	while (entries--) {
		tiff_get (base, &tag, &type, &len, &save);
		switch (tag) {
		case 1: case 3: case 5:
			gpsdata[29+tag/2] = getc(ifp);			break;
		case 2: case 4: case 7:
			FORC(6) gpsdata[tag/3*6+c] = get4();		break;
		case 6:
			FORC(2) gpsdata[18+c] = get4();			break;
		case 18: case 29:
			fgets ((char *) (gpsdata+14+tag/3), MIN(len,12), ifp);
		}
		fseek (ifp, save, SEEK_SET);
	}
}


void CLASS identify() {
	static const short pana[][6] = {
			{ 3130, 1743,  4,  0, -6,  0 },
			{ 3130, 2055,  4,  0, -6,  0 },
			{ 3130, 2319,  4,  0, -6,  0 },
			{ 3170, 2103, 18,  0,-42, 20 },
			{ 3170, 2367, 18, 13,-42,-21 },
			{ 3177, 2367,  0,  0, -1,  0 },
			{ 3304, 2458,  0,  0, -1,  0 },
			{ 3330, 2463,  9,  0, -5,  0 },
			{ 3330, 2479,  9,  0,-17,  4 },
			{ 3370, 1899, 15,  0,-44, 20 },
			{ 3370, 2235, 15,  0,-44, 20 },
			{ 3370, 2511, 15, 10,-44,-21 },
			{ 3690, 2751,  3,  0, -8, -3 },
			{ 3710, 2751,  0,  0, -3,  0 },
			{ 3724, 2450,  0,  0,  0, -2 },
			{ 3770, 2487, 17,  0,-44, 19 },
			{ 3770, 2799, 17, 15,-44,-19 },
			{ 3880, 2170,  6,  0, -6,  0 },
			{ 4060, 3018,  0,  0,  0, -2 },
			{ 4290, 2391,  3,  0, -8, -1 },
			{ 4330, 2439, 17, 15,-44,-19 },
			{ 4508, 2962,  0,  0, -3, -4 },
			{ 4508, 3330,  0,  0, -3, -6 },
	};
	static const ushort canon[][11] = {
			{ 1944, 1416,   0,  0, 48,  0 },
			{ 2144, 1560,   4,  8, 52,  2, 0, 0, 0, 25 },
			{ 2224, 1456,  48,  6,  0,  2 },
			{ 2376, 1728,  12,  6, 52,  2 },
			{ 2672, 1968,  12,  6, 44,  2 },
			{ 3152, 2068,  64, 12,  0,  0, 16 },
			{ 3160, 2344,  44, 12,  4,  4 },
			{ 3344, 2484,   4,  6, 52,  6 },
			{ 3516, 2328,  42, 14,  0,  0 },
			{ 3596, 2360,  74, 12,  0,  0 },
			{ 3744, 2784,  52, 12,  8, 12 },
			{ 3944, 2622,  30, 18,  6,  2 },
			{ 3948, 2622,  42, 18,  0,  2 },
			{ 3984, 2622,  76, 20,  0,  2, 14 },
			{ 4104, 3048,  48, 12, 24, 12 },
			{ 4116, 2178,   4,  2,  0,  0 },
			{ 4152, 2772, 192, 12,  0,  0 },
			{ 4160, 3124, 104, 11,  8, 65 },
			{ 4176, 3062,  96, 17,  8,  0, 0, 16, 0, 7, 0x49 },
			{ 4192, 3062,  96, 17, 24,  0, 0, 16, 0, 0, 0x49 },
			{ 4312, 2876,  22, 18,  0,  2 },
			{ 4352, 2874,  62, 18,  0,  0 },
			{ 4476, 2954,  90, 34,  0,  0 },
			{ 4480, 3348,  12, 10, 36, 12, 0, 0, 0, 18, 0x49 },
			{ 4480, 3366,  80, 50,  0,  0 },
			{ 4496, 3366,  80, 50, 12,  0 },
			{ 4768, 3516,  96, 16,  0,  0, 0, 16 },
			{ 4832, 3204,  62, 26,  0,  0 },
			{ 4832, 3228,  62, 51,  0,  0 },
			{ 5108, 3349,  98, 13,  0,  0 },
			{ 5120, 3318, 142, 45, 62,  0 },
			{ 5280, 3528,  72, 52,  0,  0 },
			{ 5344, 3516, 142, 51,  0,  0 },
			{ 5344, 3584, 126,100,  0,  2 },
			{ 5360, 3516, 158, 51,  0,  0 },
			{ 5568, 3708,  72, 38,  0,  0 },
			{ 5632, 3710,  96, 17,  0,  0, 0, 16, 0, 0, 0x49 },
			{ 5712, 3774,  62, 20, 10,  2 },
			{ 5792, 3804, 158, 51,  0,  0 },
			{ 5920, 3950, 122, 80,  2,  0 },
			{ 6096, 4056,  72, 34,  0,  0 },
			{ 8896, 5920, 160, 64,  0,  0 },
	};
	static const struct {
		ushort id;
		char model[20];
	} unique[] = {
			{ 0x168, "EOS 10D" },    { 0x001, "EOS-1D" },
			{ 0x175, "EOS 20D" },    { 0x174, "EOS-1D Mark II" },
			{ 0x234, "EOS 30D" },    { 0x232, "EOS-1D Mark II N" },
			{ 0x190, "EOS 40D" },    { 0x169, "EOS-1D Mark III" },
			{ 0x261, "EOS 50D" },    { 0x281, "EOS-1D Mark IV" },
			{ 0x287, "EOS 60D" },    { 0x167, "EOS-1DS" },
			{ 0x325, "EOS 70D" },
			{ 0x170, "EOS 300D" },   { 0x188, "EOS-1Ds Mark II" },
			{ 0x176, "EOS 450D" },   { 0x215, "EOS-1Ds Mark III" },
			{ 0x189, "EOS 350D" },   { 0x324, "EOS-1D C" },
			{ 0x236, "EOS 400D" },   { 0x269, "EOS-1D X" },
			{ 0x252, "EOS 500D" },   { 0x213, "EOS 5D" },
			{ 0x270, "EOS 550D" },   { 0x218, "EOS 5D Mark II" },
			{ 0x286, "EOS 600D" },   { 0x285, "EOS 5D Mark III" },
			{ 0x301, "EOS 650D" },   { 0x302, "EOS 6D" },
			{ 0x326, "EOS 700D" },   { 0x250, "EOS 7D" },
			{ 0x393, "EOS 750D" },   { 0x289, "EOS 7D Mark II" },
			{ 0x347, "EOS 760D" },
			{ 0x254, "EOS 1000D" },
			{ 0x288, "EOS 1100D" },
			{ 0x327, "EOS 1200D" },
			{ 0x346, "EOS 100D" },
	}, sonique[] = {
			{ 0x002, "DSC-R1" },     { 0x100, "DSLR-A100" },
			{ 0x101, "DSLR-A900" },  { 0x102, "DSLR-A700" },
			{ 0x103, "DSLR-A200" },  { 0x104, "DSLR-A350" },
			{ 0x105, "DSLR-A300" },  { 0x108, "DSLR-A330" },
			{ 0x109, "DSLR-A230" },  { 0x10a, "DSLR-A290" },
			{ 0x10d, "DSLR-A850" },  { 0x111, "DSLR-A550" },
			{ 0x112, "DSLR-A500" },  { 0x113, "DSLR-A450" },
			{ 0x116, "NEX-5" },      { 0x117, "NEX-3" },
			{ 0x118, "SLT-A33" },    { 0x119, "SLT-A55V" },
			{ 0x11a, "DSLR-A560" },  { 0x11b, "DSLR-A580" },
			{ 0x11c, "NEX-C3" },     { 0x11d, "SLT-A35" },
			{ 0x11e, "SLT-A65V" },   { 0x11f, "SLT-A77V" },
			{ 0x120, "NEX-5N" },     { 0x121, "NEX-7" },
			{ 0x123, "SLT-A37" },    { 0x124, "SLT-A57" },
			{ 0x125, "NEX-F3" },     { 0x126, "SLT-A99V" },
			{ 0x127, "NEX-6" },      { 0x128, "NEX-5R" },
			{ 0x129, "DSC-RX100" },  { 0x12a, "DSC-RX1" },
			{ 0x12e, "ILCE-3000" },  { 0x12f, "SLT-A58" },
			{ 0x131, "NEX-3N" },     { 0x132, "ILCE-7" },
			{ 0x133, "NEX-5T" },     { 0x134, "DSC-RX100M2" },
			{ 0x135, "DSC-RX10" },   { 0x136, "DSC-RX1R" },
			{ 0x137, "ILCE-7R" },    { 0x138, "ILCE-6000" },
			{ 0x139, "ILCE-5000" },  { 0x13d, "DSC-RX100M3" },
			{ 0x13e, "ILCE-7S" },    { 0x13f, "ILCA-77M2" },
			{ 0x153, "ILCE-5100" },  { 0x154, "ILCE-7M2" },
			{ 0x15a, "ILCE-QX1" },
	};
	static const struct {
		unsigned fsize;
		ushort rw, rh;
		uchar lm, tm, rm, bm, lf, cf, max, flags;
		char make[10], model[20];
		ushort offset;
	} table[] = {
			{   786432,1024, 768, 0, 0, 0, 0, 0,0x94,0,0,"AVT","F-080C" },
			{  1447680,1392,1040, 0, 0, 0, 0, 0,0x94,0,0,"AVT","F-145C" },
			{  1920000,1600,1200, 0, 0, 0, 0, 0,0x94,0,0,"AVT","F-201C" },
			{  5067304,2588,1958, 0, 0, 0, 0, 0,0x94,0,0,"AVT","F-510C" },
			{  5067316,2588,1958, 0, 0, 0, 0, 0,0x94,0,0,"AVT","F-510C",12 },
			{ 10134608,2588,1958, 0, 0, 0, 0, 9,0x94,0,0,"AVT","F-510C" },
			{ 10134620,2588,1958, 0, 0, 0, 0, 9,0x94,0,0,"AVT","F-510C",12 },
			{ 16157136,3272,2469, 0, 0, 0, 0, 9,0x94,0,0,"AVT","F-810C" },
			{ 15980544,3264,2448, 0, 0, 0, 0, 8,0x61,0,1,"AgfaPhoto","DC-833m" },
			{  9631728,2532,1902, 0, 0, 0, 0,96,0x61,0,0,"Alcatel","5035D" },
			{  2868726,1384,1036, 0, 0, 0, 0,64,0x49,0,8,"Baumer","TXG14",1078 },
			{  5298000,2400,1766,12,12,44, 2,40,0x94,0,2,"Canon","PowerShot SD300" },
			{  6553440,2664,1968, 4, 4,44, 4,40,0x94,0,2,"Canon","PowerShot A460" },
			{  6573120,2672,1968,12, 8,44, 0,40,0x94,0,2,"Canon","PowerShot A610" },
			{  6653280,2672,1992,10, 6,42, 2,40,0x94,0,2,"Canon","PowerShot A530" },
			{  7710960,2888,2136,44, 8, 4, 0,40,0x94,0,2,"Canon","PowerShot S3 IS" },
			{  9219600,3152,2340,36,12, 4, 0,40,0x94,0,2,"Canon","PowerShot A620" },
			{  9243240,3152,2346,12, 7,44,13,40,0x49,0,2,"Canon","PowerShot A470" },
			{ 10341600,3336,2480, 6, 5,32, 3,40,0x94,0,2,"Canon","PowerShot A720 IS" },
			{ 10383120,3344,2484,12, 6,44, 6,40,0x94,0,2,"Canon","PowerShot A630" },
			{ 12945240,3736,2772,12, 6,52, 6,40,0x94,0,2,"Canon","PowerShot A640" },
			{ 15636240,4104,3048,48,12,24,12,40,0x94,0,2,"Canon","PowerShot A650" },
			{ 15467760,3720,2772, 6,12,30, 0,40,0x94,0,2,"Canon","PowerShot SX110 IS" },
			{ 15534576,3728,2778,12, 9,44, 9,40,0x94,0,2,"Canon","PowerShot SX120 IS" },
			{ 18653760,4080,3048,24,12,24,12,40,0x94,0,2,"Canon","PowerShot SX20 IS" },
			{ 19131120,4168,3060,92,16, 4, 1,40,0x94,0,2,"Canon","PowerShot SX220 HS" },
			{ 21936096,4464,3276,25,10,73,12,40,0x16,0,2,"Canon","PowerShot SX30 IS" },
			{ 24724224,4704,3504, 8,16,56, 8,40,0x94,0,2,"Canon","PowerShot A3300 IS" },
			{  1976352,1632,1211, 0, 2, 0, 1, 0,0x94,0,1,"Casio","QV-2000UX" },
			{  3217760,2080,1547, 0, 0,10, 1, 0,0x94,0,1,"Casio","QV-3*00EX" },
			{  6218368,2585,1924, 0, 0, 9, 0, 0,0x94,0,1,"Casio","QV-5700" },
			{  7816704,2867,2181, 0, 0,34,36, 0,0x16,0,1,"Casio","EX-Z60" },
			{  2937856,1621,1208, 0, 0, 1, 0, 0,0x94,7,13,"Casio","EX-S20" },
			{  4948608,2090,1578, 0, 0,32,34, 0,0x94,7,1,"Casio","EX-S100" },
			{  6054400,2346,1720, 2, 0,32, 0, 0,0x94,7,1,"Casio","QV-R41" },
			{  7426656,2568,1928, 0, 0, 0, 0, 0,0x94,0,1,"Casio","EX-P505" },
			{  7530816,2602,1929, 0, 0,22, 0, 0,0x94,7,1,"Casio","QV-R51" },
			{  7542528,2602,1932, 0, 0,32, 0, 0,0x94,7,1,"Casio","EX-Z50" },
			{  7562048,2602,1937, 0, 0,25, 0, 0,0x16,7,1,"Casio","EX-Z500" },
			{  7753344,2602,1986, 0, 0,32,26, 0,0x94,7,1,"Casio","EX-Z55" },
			{  9313536,2858,2172, 0, 0,14,30, 0,0x94,7,1,"Casio","EX-P600" },
			{ 10834368,3114,2319, 0, 0,27, 0, 0,0x94,0,1,"Casio","EX-Z750" },
			{ 10843712,3114,2321, 0, 0,25, 0, 0,0x94,0,1,"Casio","EX-Z75" },
			{ 10979200,3114,2350, 0, 0,32,32, 0,0x94,7,1,"Casio","EX-P700" },
			{ 12310144,3285,2498, 0, 0, 6,30, 0,0x94,0,1,"Casio","EX-Z850" },
			{ 12489984,3328,2502, 0, 0,47,35, 0,0x94,0,1,"Casio","EX-Z8" },
			{ 15499264,3754,2752, 0, 0,82, 0, 0,0x94,0,1,"Casio","EX-Z1050" },
			{ 18702336,4096,3044, 0, 0,24, 0,80,0x94,7,1,"Casio","EX-ZR100" },
			{  7684000,2260,1700, 0, 0, 0, 0,13,0x94,0,1,"Casio","QV-4000" },
			{   787456,1024, 769, 0, 1, 0, 0, 0,0x49,0,0,"Creative","PC-CAM 600" },
			{ 28829184,4384,3288, 0, 0, 0, 0,36,0x61,0,0,"DJI" },
			{ 15151104,4608,3288, 0, 0, 0, 0, 0,0x94,0,0,"Matrix" },
			{  3840000,1600,1200, 0, 0, 0, 0,65,0x49,0,0,"Foculus","531C" },
			{   307200, 640, 480, 0, 0, 0, 0, 0,0x94,0,0,"Generic" },
			{    62464, 256, 244, 1, 1, 6, 1, 0,0x8d,0,0,"Kodak","DC20" },
			{   124928, 512, 244, 1, 1,10, 1, 0,0x8d,0,0,"Kodak","DC20" },
			{  1652736,1536,1076, 0,52, 0, 0, 0,0x61,0,0,"Kodak","DCS200" },
			{  4159302,2338,1779, 1,33, 1, 2, 0,0x94,0,0,"Kodak","C330" },
			{  4162462,2338,1779, 1,33, 1, 2, 0,0x94,0,0,"Kodak","C330",3160 },
			{  2247168,1232, 912, 0, 0,16, 0, 0,0x00,0,0,"Kodak","C330" },
			{  3370752,1232, 912, 0, 0,16, 0, 0,0x00,0,0,"Kodak","C330" },
			{  6163328,2864,2152, 0, 0, 0, 0, 0,0x94,0,0,"Kodak","C603" },
			{  6166488,2864,2152, 0, 0, 0, 0, 0,0x94,0,0,"Kodak","C603",3160 },
			{   460800, 640, 480, 0, 0, 0, 0, 0,0x00,0,0,"Kodak","C603" },
			{  9116448,2848,2134, 0, 0, 0, 0, 0,0x00,0,0,"Kodak","C603" },
			{ 12241200,4040,3030, 2, 0, 0,13, 0,0x49,0,0,"Kodak","12MP" },
			{ 12272756,4040,3030, 2, 0, 0,13, 0,0x49,0,0,"Kodak","12MP",31556 },
			{ 18000000,4000,3000, 0, 0, 0, 0, 0,0x00,0,0,"Kodak","12MP" },
			{   614400, 640, 480, 0, 3, 0, 0,64,0x94,0,0,"Kodak","KAI-0340" },
			{ 15360000,3200,2400, 0, 0, 0, 0,96,0x16,0,0,"Lenovo","A820" },
			{  3884928,1608,1207, 0, 0, 0, 0,96,0x16,0,0,"Micron","2010",3212 },
			{  1138688,1534, 986, 0, 0, 0, 0, 0,0x61,0,0,"Minolta","RD175",513 },
			{  1581060,1305, 969, 0, 0,18, 6, 6,0x1e,4,1,"Nikon","E900" },
			{  2465792,1638,1204, 0, 0,22, 1, 6,0x4b,5,1,"Nikon","E950" },
			{  2940928,1616,1213, 0, 0, 0, 7,30,0x94,0,1,"Nikon","E2100" },
			{  4771840,2064,1541, 0, 0, 0, 1, 6,0xe1,0,1,"Nikon","E990" },
			{  4775936,2064,1542, 0, 0, 0, 0,30,0x94,0,1,"Nikon","E3700" },
			{  5865472,2288,1709, 0, 0, 0, 1, 6,0xb4,0,1,"Nikon","E4500" },
			{  5869568,2288,1710, 0, 0, 0, 0, 6,0x16,0,1,"Nikon","E4300" },
			{  7438336,2576,1925, 0, 0, 0, 1, 6,0xb4,0,1,"Nikon","E5000" },
			{  8998912,2832,2118, 0, 0, 0, 0,30,0x94,7,1,"Nikon","COOLPIX S6" },
			{  5939200,2304,1718, 0, 0, 0, 0,30,0x16,0,0,"Olympus","C770UZ" },
			{  3178560,2064,1540, 0, 0, 0, 0, 0,0x94,0,1,"Pentax","Optio S" },
			{  4841984,2090,1544, 0, 0,22, 0, 0,0x94,7,1,"Pentax","Optio S" },
			{  6114240,2346,1737, 0, 0,22, 0, 0,0x94,7,1,"Pentax","Optio S4" },
			{ 10702848,3072,2322, 0, 0, 0,21,30,0x94,0,1,"Pentax","Optio 750Z" },
			{  4147200,1920,1080, 0, 0, 0, 0, 0,0x49,0,0,"Photron","BC2-HD" },
			{  4151666,1920,1080, 0, 0, 0, 0, 0,0x49,0,0,"Photron","BC2-HD",8 },
			{ 13248000,2208,3000, 0, 0, 0, 0,13,0x61,0,0,"Pixelink","A782" },
			{  6291456,2048,1536, 0, 0, 0, 0,96,0x61,0,0,"RoverShot","3320AF" },
			{   311696, 644, 484, 0, 0, 0, 0, 0,0x16,0,8,"ST Micro","STV680 VGA" },
			{ 16098048,3288,2448, 0, 0,24, 0, 9,0x94,0,1,"Samsung","S85" },
			{ 16215552,3312,2448, 0, 0,48, 0, 9,0x94,0,1,"Samsung","S85" },
			{ 20487168,3648,2808, 0, 0, 0, 0,13,0x94,5,1,"Samsung","WB550" },
			{ 24000000,4000,3000, 0, 0, 0, 0,13,0x94,5,1,"Samsung","WB550" },
			{ 12582980,3072,2048, 0, 0, 0, 0,33,0x61,0,0,"Sinar","",68 },
			{ 33292868,4080,4080, 0, 0, 0, 0,33,0x61,0,0,"Sinar","",68 },
			{ 44390468,4080,5440, 0, 0, 0, 0,33,0x61,0,0,"Sinar","",68 },
			{  1409024,1376,1024, 0, 0, 1, 0, 0,0x49,0,0,"Sony","XCD-SX910CR" },
			{  2818048,1376,1024, 0, 0, 1, 0,97,0x49,0,0,"Sony","XCD-SX910CR" },
	};
	static const char *corp[] =
	{ "AgfaPhoto", "Canon", "Casio", "Epson", "Fujifilm",
			"Mamiya", "Minolta", "Motorola", "Kodak", "Konica", "Leica",
			"Nikon", "Nokia", "Olympus", "Pentax", "Phase One", "Ricoh",
			"Samsung", "Sigma", "Sinar", "Sony" };
	char head[32], *cp;
	int hlen, flen, fsize, zero_fsize=1, i, c;
	struct jhead jh;

	//copied from global scope
	char *meta_data, xtrans[6][6], xtrans_abs[6][6];
	char cdesc[5];

	unsigned *oprof, fuji_layout, multi_out=0;
	unsigned mix_green, raw_color, zero_is_bad;
	unsigned load_flags;

	unsigned thumb_misc;
	ushort shrink, iheight, iwidth, fuji_width, thumb_width, thumb_height;
	ushort *raw_image, (*image)[4];
	ushort white[8][8], cr2_slice[3], sraw_mul[4];
	double aber[4]={1,1,1,1}, gamm[6]={ 0.45,4.5,0,0,0,0 };
	float bright=1, user_mul[4]={0,0,0,0}, threshold=0;
	int mask[8][4];
	int half_size=0, four_color_rgb=0, document_mode=0, highlight=0;
	int verbose=0, use_auto_wb=0, use_camera_wb=0, use_camera_matrix=1;
	int output_color=1, output_bps=8, output_tiff=0, med_passes=0;
	int no_auto_bright=0;
	unsigned greybox[4] = { 0, 0, UINT_MAX, UINT_MAX };
	float cmatrix[3][4], rgb_cam[3][4];
	const double xyz_rgb[3][3] = {			/* XYZ from RGB */
			{ 0.412453, 0.357580, 0.180423 },
			{ 0.212671, 0.715160, 0.072169 },
			{ 0.019334, 0.119193, 0.950227 } };

	int histogram[4][0x2000];
	void (*write_thumb)(), (*write_fun)();
	jmp_buf failure;

	struct decode {
		struct decode *branch[2];
		int leaf;
	} first_decode[2048], *second_decode, *free_decode;


	struct ph1 {
		int format, key_off, tag_21a;
		int black, split_col, black_col, split_row, black_row;
		float tag_210;
	} ph1;

	//#define FORCC FORC(colors)

	tiff_flip = dcraw_flip = filters = UINT_MAX;	/* unknown */
	raw_height = raw_width = fuji_width = fuji_layout = cr2_slice[0] = 0;
	maximum = height = width = top_margin = left_margin = 0;
	cdesc[0] = desc[0] = artist[0] = make[0] = model[0] = model2[0] = 0;
	iso_speed = shutter = aperture = focal_len = unique_id = 0;
	tiff_nifds = 0;
	memset (tiff_ifd, 0, sizeof tiff_ifd);
	memset (gpsdata, 0, sizeof gpsdata);
	memset (cblack, 0, sizeof cblack);
	memset (white, 0, sizeof white);
	memset (mask, 0, sizeof mask);
	thumb_offset = thumb_length = thumb_width = thumb_height = 0;
	load_raw = 0; //thumb_load_raw = 0;
	//write_thumb = &CLASS jpeg_thumb;
	data_offset = meta_offset = meta_length = tiff_bps = tiff_compress = 0;
	kodak_cbpp = zero_after_ff = dng_version = load_flags = 0;
	timestamp = shot_order = tiff_samples = black = is_foveon = 0;
	mix_green = profile_length = data_error = zero_is_bad = 0;
	pixel_aspect = is_raw = raw_color = 1;
	tile_width = tile_length = 0;
	for (i=0; i < 4; i++) {
		cam_mul[i] = i == 1;
		pre_mul[i] = i < 3;
		FORC3 cmatrix[c][i] = 0;
		FORC3 rgb_cam[c][i] = c == i;
	}
	colors = 3;
	for (i=0; i < 0x10000; i++) curve[i] = i;

	order = get2();
	hlen = get4();
	fseek (ifp, 0, SEEK_SET);
	fread (head, 1, 32, ifp);
	fseek (ifp, 0, SEEK_END);
	flen = fsize = ftell(ifp);
	if ((cp = (char *) memmem (head, 32, "MMMM", 4)) ||
			(cp = (char *) memmem (head, 32, "IIII", 4))) {
		cout << ERROR_LINE << "Not implemented yet." << endl;
		//		parse_phase_one (cp-head);
		//		if (cp-head && parse_tiff(0)) apply_tiff();
	} else if (order == 0x4949 || order == 0x4d4d) {
		if (!memcmp (head+6,"HEAPCCDR",8)) {
			cout << ERROR_LINE << "Not implemented yet." << endl;
			//			data_offset = hlen;
			//			parse_ciff (hlen, flen-hlen, 0);
			//			load_raw = &CLASS canon_load_raw;
		}
		else if (parse_tiff(0)) { apply_tiff(); }
	}
	/*else if (!memcmp (head,"\xff\xd8\xff\xe1",4) &&
			!memcmp (head+6,"Exif",4)) {
		cout << ERROR_LINE << "Not implemented yet." << endl;

		//		fseek (ifp, 4, SEEK_SET);
		//		data_offset = 4 + get2();
		//		fseek (ifp, data_offset, SEEK_SET);
		//		if (fgetc(ifp) != 0xff)
		//			parse_tiff(12);
		//		thumb_offset = 0;
	}
	else if (!memcmp (head+25,"ARECOYK",7)) {
		strcpy (make, "Contax");
		strcpy (model,"N Digital");
		fseek (ifp, 33, SEEK_SET);
		get_timestamp(1);
		fseek (ifp, 60, SEEK_SET);
		FORC4 cam_mul[c ^ (c >> 1)] = get4();
	} else if (!strcmp (head, "PXN")) {
		strcpy (make, "Logitech");
		strcpy (model,"Fotoman Pixtura");
	} else if (!strcmp (head, "qktk")) {
		strcpy (make, "Apple");
		strcpy (model,"QuickTake 100");
		load_raw = &CLASS quicktake_100_load_raw;
	} else if (!strcmp (head, "qktn")) {
		strcpy (make, "Apple");
		strcpy (model,"QuickTake 150");
		load_raw = &CLASS kodak_radc_load_raw;
	}
	else if (!memcmp (head,"FUJIFILM",8)) {
		fseek (ifp, 84, SEEK_SET);
		thumb_offset = get4();
		thumb_length = get4();
		fseek (ifp, 92, SEEK_SET);
		parse_fuji (get4());
		if (thumb_offset > 120) {
			fseek (ifp, 120, SEEK_SET);
			is_raw += (i = get4()) && 1;
			if (is_raw == 2 && shot_select)
				parse_fuji (i);
		}
		load_raw = &CLASS unpacked_load_raw;
		fseek (ifp, 100+28*(shot_select > 0), SEEK_SET);
		parse_tiff (data_offset = get4());
		parse_tiff (thumb_offset+12);
		apply_tiff();
	} else if (!memcmp (head,"RIFF",4)) {
		fseek (ifp, 0, SEEK_SET);
		parse_riff();
	} else if (!memcmp (head+4,"ftypqt   ",9)) {
		fseek (ifp, 0, SEEK_SET);
		parse_qt (fsize);
		is_raw = 0;
	} else if (!memcmp (head,"\0\001\0\001\0@",6)) {
		fseek (ifp, 6, SEEK_SET);
		fread (make, 1, 8, ifp);
		fread (model, 1, 8, ifp);
		fread (model2, 1, 16, ifp);
		data_offset = get2();
		get2();
		raw_width = get2();
		raw_height = get2();
		load_raw = &CLASS nokia_load_raw;
		filters = 0x61616161;
	} else if (!memcmp (head,"NOKIARAW",8)) {
		strcpy (make, "NOKIA");
		order = 0x4949;
		fseek (ifp, 300, SEEK_SET);
		data_offset = get4();
		i = get4();
		width = get2();
		height = get2();
		switch (tiff_bps = i*8 / (width * height)) {
		case  8: load_raw = &CLASS eight_bit_load_raw;  break;
		case 10: load_raw = &CLASS nokia_load_raw;
		}
		raw_height = height + (top_margin = i / (width * tiff_bps/8) - height);
		mask[0][3] = 1;
		filters = 0x61616161;
	} else if (!memcmp (head,"ARRI",4)) {
		order = 0x4949;
		fseek (ifp, 20, SEEK_SET);
		width = get4();
		height = get4();
		strcpy (make, "ARRI");
		fseek (ifp, 668, SEEK_SET);
		fread (model, 1, 64, ifp);
		data_offset = 4096;
		load_raw = &CLASS packed_load_raw;
		load_flags = 88;
		filters = 0x61616161;
	} else if (!memcmp (head,"XPDS",4)) {
		order = 0x4949;
		fseek (ifp, 0x800, SEEK_SET);
		fread (make, 1, 41, ifp);
		raw_height = get2();
		raw_width  = get2();
		fseek (ifp, 56, SEEK_CUR);
		fread (model, 1, 30, ifp);
		data_offset = 0x10000;
		load_raw = &CLASS canon_rmf_load_raw;
		gamma_curve (0, 12.25, 1, 1023);
	} else if (!memcmp (head+4,"RED1",4)) {
		strcpy (make, "Red");
		strcpy (model,"One");
		parse_redcine();
		load_raw = &CLASS redcine_load_raw;
		gamma_curve (1/2.4, 12.92, 1, 4095);
		filters = 0x49494949;
	} else if (!memcmp (head,"DSC-Image",9))
		parse_rollei();
	else if (!memcmp (head,"PWAD",4))
		parse_sinar_ia();
	else if (!memcmp (head,"\0MRM",4))
		parse_minolta(0);
	else if (!memcmp (head,"FOVb",4))
		parse_foveon();
	else if (!memcmp (head,"CI",2))
		parse_cine();
	 */

	if (make[0] == 0) {
		cout << ERROR_LINE << "Not implemented yet." << endl;
		exit(0);
		//		for (zero_fsize=i=0; i < sizeof table / sizeof *table; i++) {
		//			if (fsize == table[i].fsize) {
		//				strcpy (make,  table[i].make );
		//				strcpy (model, table[i].model);
		//				flip = table[i].flags >> 2;
		//				zero_is_bad = table[i].flags & 2;
		//				if (table[i].flags & 1)
		//					parse_external_jpeg();
		//				data_offset = table[i].offset;
		//				raw_width   = table[i].rw;
		//				raw_height  = table[i].rh;
		//				left_margin = table[i].lm;
		//				top_margin = table[i].tm;
		//				width  = raw_width - left_margin - table[i].rm;
		//				height = raw_height - top_margin - table[i].bm;
		//				filters = 0x1010101 * table[i].cf;
		//				colors = 4 - !((filters & filters >> 1) & 0x5555);
		//				load_flags = table[i].lf;
		//				switch (tiff_bps = (fsize-data_offset)*8 / (raw_width*raw_height)) {
		//				case 6:
		//					load_raw = &CLASS minolta_rd175_load_raw;  break;
		//				case 8:
		//					load_raw = &CLASS eight_bit_load_raw;  break;
		//				case 10: case 12:
		//					load_flags |= 128;
		//					load_raw = &CLASS packed_load_raw;     break;
		//				case 16:
		//					order = 0x4949 | 0x404 * (load_flags & 1);
		//					tiff_bps -= load_flags >> 4;
		//					tiff_bps -= load_flags = load_flags >> 1 & 7;
		//					load_raw = &CLASS unpacked_load_raw;
		//				}
		//				maximum = (1 << tiff_bps) - (1 << table[i].max);
		//			}
		//		}
	} //end ofif (make[0] == 0) {

	if (zero_fsize) fsize = 0;
	if (make[0] == 0) {
		cout << ERROR_LINE << "Not implemented yet." << endl;
		exit(0);
		//		parse_smal (0, flen);
		//		parse_jpeg(0);
		//		if (!(strncmp(model,"ov",2) && strncmp(model,"RP_OV",5)) &&
		//				!fseek (ifp, -6404096, SEEK_END) &&
		//				fread (head, 1, 32, ifp) && !strcmp(head,"BRCMn")) {
		//			strcpy (make, "OmniVision");
		//			data_offset = ftell(ifp) + 0x8000-32;
		//			width = raw_width;
		//			raw_width = 2611;
		//			load_raw = &CLASS nokia_load_raw;
		//			filters = 0x16161616;
		//		} else is_raw = 0;
	}

	for (i=0; i < sizeof corp / sizeof *corp; i++) {
		if (strcasestr (make, corp[i]))	{/* Simplify company names */
			strcpy (make, corp[i]);
		}
	}

	//	if ((!strcmp(make,"Kodak") || !strcmp(make,"Leica")) &&
	//			((cp = strcasestr(model," DIGITAL CAMERA")) ||
	//					(cp = strstr(model,"FILE VERSION"))))
	//		*cp = 0;

	//	if (!strncasecmp(model,"PENTAX",6))
	//		strcpy (make, "Pentax");

	cp = make + strlen(make);		/* Remove trailing spaces */
	while (*--cp == ' ') *cp = 0;
	cp = model + strlen(model);
	while (*--cp == ' ') *cp = 0;
	i = strlen(make);			/* Remove make from model */
	if (!strncasecmp (model, make, i) && model[i++] == ' ') {
		memmove (model, model+i, 64-i);
	}

	//	if (!strncmp (model,"FinePix ",8))
	//		strcpy (model, model+8);
	//	if (!strncmp (model,"Digital Camera ",15))
	//		strcpy (model, model+15);

	desc[511] = artist[63] = make[63] = model[63] = model2[63] = 0;
	if (!is_raw) {
		cerr << ERROR_LINE << "!is_raw" << endl; exit(0);
	}

	if (!height) height = raw_height;
	if (!width)  width  = raw_width;

	//	if (height == 2624 && width == 3936)	/* Pentax K10D and Samsung GX10 */
	//	{ height  = 2616;   width  = 3896; }
	//	if (height == 3136 && width == 4864)  /* Pentax K20D and Samsung GX20 */
	//	{ height  = 3124;   width  = 4688; filters = 0x16161616; }
	//	if (width == 4352 && (!strcmp(model,"K-r") || !strcmp(model,"K-x")))
	//	{			width  = 4309; filters = 0x16161616; }
	//	if (width >= 4960 && !strncmp(model,"K-5",3))
	//	{ left_margin = 10; width  = 4950; filters = 0x16161616; }
	//	if (width == 4736 && !strcmp(model,"K-7"))
	//	{ height  = 3122;   width  = 4684; filters = 0x16161616; top_margin = 2; }
	//	if (width == 6080 && !strcmp(model,"K-3"))
	//	{ left_margin = 4;  width  = 6040; }
	//	if (width == 7424 && !strcmp(model,"645D"))
	//	{ height  = 5502;   width  = 7328; filters = 0x61616161; top_margin = 29;
	//	left_margin = 48; }
	//	if (height == 3014 && width == 4096)	/* Ricoh GX200 */
	//		width  = 4014;

	if (dng_version) {
		cout << ERROR_LINE << "Not implemented yet." << endl; exit(0);
		//		if (filters == UINT_MAX) filters = 0;
		//		if (filters) is_raw *= tiff_samples;
		//		else	 colors  = tiff_samples;
		//		switch (tiff_compress) {
		//		case 0:
		//		case 1:     load_raw = &CLASS   packed_dng_load_raw;  break;
		//		case 7:     load_raw = &CLASS lossless_dng_load_raw;  break;
		//		case 34892: load_raw = &CLASS    lossy_dng_load_raw;  break;
		//		default:    load_raw = 0;
		//		}
		//		goto dng_skip;
	}

	if (!strcmp(make,"Canon") && !fsize && tiff_bps != 15) {
		cout << ERROR_LINE << "Not implemented yet." << endl; exit(0);
		//		if (!load_raw)
		//			load_raw = &CLASS lossless_jpeg_load_raw;
		//		for (i=0; i < sizeof canon / sizeof *canon; i++)
		//			if (raw_width == canon[i][0] && raw_height == canon[i][1]) {
		//				width  = raw_width - (left_margin = canon[i][2]);
		//				height = raw_height - (top_margin = canon[i][3]);
		//				width  -= canon[i][4];
		//				height -= canon[i][5];
		//				mask[0][1] =  canon[i][6];
		//				mask[0][3] = -canon[i][7];
		//				mask[1][1] =  canon[i][8];
		//				mask[1][3] = -canon[i][9];
		//				if (canon[i][10]) filters = canon[i][10] * 0x01010101;
		//			}
		//		if ((unique_id | 0x20000) == 0x2720000) {
		//			left_margin = 8;
		//			top_margin = 16;
		//		}
	}

	for (i=0; i < sizeof unique / sizeof *unique; i++) {
		if (unique_id == 0x80000000 + unique[i].id) {
			cout << ERROR_LINE << "Not implemented yet." << endl; exit(0);
			//			adobe_coeff ("Canon", unique[i].model);
			//			if (model[4] == 'K' && strlen(model) == 8)
			//				strcpy (model, unique[i].model);
		}
	}
	for (i=0; i < sizeof sonique / sizeof *sonique; i++) {
		if (unique_id == sonique[i].id) {
			strcpy (model, sonique[i].model);
		}
	}
	if (!strcmp(make,"Nikon")) {
		if (!load_raw) {
			cout << ERROR_LINE << "Not implemented yet." << endl; exit(0);
			//load_raw = &CLASS packed_load_raw;
		}
		if (model[0] == 'E') {
			cout << ERROR_LINE << "Not implemented yet." << endl; exit(0);
			//load_flags |= !data_offset << 2 | 2;
		}
	}


	/* Set parameters based on camera name (for non-DNG files). */
	//
	//	if (!strcmp(model,"KAI-0340")
	//			&& find_green (16, 16, 3840, 5120) < 25) {
	//		height = 480;
	//		top_margin = filters = 0;
	//		strcpy (model,"C603");
	//	}
	//	if (is_foveon) {
	//		if (height*2 < width) pixel_aspect = 0.5;
	//		if (height   > width) pixel_aspect = 2;
	//		filters = 0;
	//		simple_coeff(0);
	//	} else if (!strcmp(make,"Canon") && tiff_bps == 15) {
	//		switch (width) {
	//		case 3344: width -= 66;
	//		case 3872: width -= 6;
	//		}
	//		if (height > width) {
	//			SWAP(height,width);
	//			SWAP(raw_height,raw_width);
	//		}
	//		if (width == 7200 && height == 3888) {
	//			raw_width  = width  = 6480;
	//			raw_height = height = 4320;
	//		}
	//		filters = 0;
	//		tiff_samples = colors = 3;
	//		load_raw = &CLASS canon_sraw_load_raw;
	//	} else if (!strcmp(model,"PowerShot 600")) {
	//		height = 613;
	//		width  = 854;
	//		raw_width = 896;
	//		colors = 4;
	//		filters = 0xe1e4e1e4;
	//		load_raw = &CLASS canon_600_load_raw;
	//	} else if (!strcmp(model,"PowerShot A5") ||
	//			!strcmp(model,"PowerShot A5 Zoom")) {
	//		height = 773;
	//		width  = 960;
	//		raw_width = 992;
	//		pixel_aspect = 256/235.0;
	//		filters = 0x1e4e1e4e;
	//		goto canon_a5;
	//	} else if (!strcmp(model,"PowerShot A50")) {
	//		height =  968;
	//		width  = 1290;
	//		raw_width = 1320;
	//		filters = 0x1b4e4b1e;
	//		goto canon_a5;
	//	} else if (!strcmp(model,"PowerShot Pro70")) {
	//		height = 1024;
	//		width  = 1552;
	//		filters = 0x1e4b4e1b;
	//		canon_a5:
	//		colors = 4;
	//		tiff_bps = 10;
	//		load_raw = &CLASS packed_load_raw;
	//		load_flags = 40;
	//	} else if (!strcmp(model,"PowerShot Pro90 IS") ||
	//			!strcmp(model,"PowerShot G1")) {
	//		colors = 4;
	//		filters = 0xb4b4b4b4;
	//	} else if (!strcmp(model,"PowerShot A610")) {
	//		if (canon_s2is()) strcpy (model+10, "S2 IS");
	//	} else if (!strcmp(model,"PowerShot SX220 HS")) {
	//		mask[1][3] = -4;
	//	} else if (!strcmp(model,"EOS D2000C")) {
	//		filters = 0x61616161;
	//		black = curve[200];
	//	}
	////Nikon
	//else if (!strcmp(model,"D1")) {
	//		cam_mul[0] *= 256/527.0;
	//		cam_mul[2] *= 256/317.0;
	//	} else if (!strcmp(model,"D1X")) {
	//		width -= 4;
	//		pixel_aspect = 0.5;
	//	} else if (!strcmp(model,"D40X") ||
	//			!strcmp(model,"D60")  ||
	//			!strcmp(model,"D80")  ||
	//			!strcmp(model,"D3000")) {
	//		height -= 3;
	//		width  -= 4;
	//	} else if (!strcmp(model,"D3")   ||
	//			!strcmp(model,"D3S")  ||
	//			!strcmp(model,"D700")) {
	//		width -= 4;
	//		left_margin = 2;
	//	} else if (!strcmp(model,"D3100")) {
	//		width -= 28;
	//		left_margin = 6;
	//	} else if (!strcmp(model,"D5000") ||
	//			!strcmp(model,"D90")) {
	//		width -= 42;
	//	} else if (!strcmp(model,"D5100") ||
	//			!strcmp(model,"D7000") ||
	//			!strcmp(model,"COOLPIX A")) {
	//		width -= 44;
	//	} else if (!strcmp(model,"D3200") ||
	//			!strncmp(model,"D6",2)  ||
	//			!strncmp(model,"D800",4)) {
	//		width -= 46;
	//	} else if (!strcmp(model,"D4") ||
	//			!strcmp(model,"Df")) {
	//		width -= 52;
	//		left_margin = 2;
	//	} else if (!strncmp(model,"D40",3) ||
	//			!strncmp(model,"D50",3) ||
	//			!strncmp(model,"D70",3)) {
	//		width--;
	//	} else if (!strcmp(model,"D100")) {
	//		if (load_flags)
	//			raw_width = (width += 3) + 3;
	//	} else if (!strcmp(model,"D200")) {
	//		left_margin = 1;
	//		width -= 4;
	//		filters = 0x94949494;
	//	} else if (!strncmp(model,"D2H",3)) {
	//		left_margin = 6;
	//		width -= 14;
	//	} else if (!strncmp(model,"D2X",3)) {
	//		if (width == 3264) width -= 32;
	//		else width -= 8;
	//	} else if (!strncmp(model,"D300",4)) {
	//		width -= 32;
	//	} else if (!strncmp(model,"COOLPIX P",9) && raw_width != 4032) {
	//		load_flags = 24;
	//		filters = 0x94949494;
	//		if (model[9] == '7' && iso_speed >= 400)
	//			black = 255;
	//	} else if (!strncmp(model,"1 ",2)) {
	//		height -= 2;
	//	} else if (fsize == 1581060) {
	//		simple_coeff(3);
	//		pre_mul[0] = 1.2085;
	//		pre_mul[1] = 1.0943;
	//		pre_mul[3] = 1.1103;
	//	} else if (fsize == 3178560) {
	//		cam_mul[0] *= 4;
	//		cam_mul[2] *= 4;
	//	} else if (fsize == 4771840) {
	//		if (!timestamp && nikon_e995())
	//			strcpy (model, "E995");
	//		if (strcmp(model,"E995")) {
	//			filters = 0xb4b4b4b4;
	//			simple_coeff(3);
	//			pre_mul[0] = 1.196;
	//			pre_mul[1] = 1.246;
	//			pre_mul[2] = 1.018;
	//		}
	//	} else if (fsize == 2940928) {
	//		if (!timestamp && !nikon_e2100())
	//			strcpy (model,"E2500");
	//		if (!strcmp(model,"E2500")) {
	//			height -= 2;
	//			load_flags = 6;
	//			colors = 4;
	//			filters = 0x4b4b4b4b;
	//		}
	//	} else if (fsize == 4775936) {
	//		if (!timestamp) nikon_3700();
	//		if (model[0] == 'E' && atoi(model+1) < 3700)
	//			filters = 0x49494949;
	//		if (!strcmp(model,"Optio 33WR")) {
	//			flip = 1;
	//			filters = 0x16161616;
	//		}
	//		if (make[0] == 'O') {
	//			i = find_green (12, 32, 1188864, 3576832);
	//			c = find_green (12, 32, 2383920, 2387016);
	//			if (abs(i) < abs(c)) {
	//				SWAP(i,c);
	//				load_flags = 24;
	//			}
	//			if (i < 0) filters = 0x61616161;
	//		}
	//	} else if (fsize == 5869568) {
	//		if (!timestamp && minolta_z2()) {
	//			strcpy (make, "Minolta");
	//			strcpy (model,"DiMAGE Z2");
	//		}
	//		load_flags = 6 + 24*(make[0] == 'M');
	//	} else if (fsize == 6291456) {
	//		fseek (ifp, 0x300000, SEEK_SET);
	//		if ((order = guess_byte_order(0x10000)) == 0x4d4d) {
	//			height -= (top_margin = 16);
	//			width -= (left_margin = 28);
	//			maximum = 0xf5c0;
	//			strcpy (make, "ISG");
	//			model[0] = 0;
	//		}
	//	} else if (!strcmp(make,"Fujifilm")) {
	//		if (!strcmp(model+7,"S2Pro")) {
	//			strcpy (model,"S2Pro");
	//			height = 2144;
	//			width  = 2880;
	//			flip = 6;
	//		} else if (load_raw != &CLASS packed_load_raw)
	//			maximum = (is_raw == 2 && shot_select) ? 0x2f00 : 0x3e00;
	//		top_margin = (raw_height - height) >> 2 << 1;
	//		left_margin = (raw_width - width ) >> 2 << 1;
	//		if (width == 2848 || width == 3664) filters = 0x16161616;
	//		if (width == 4032 || width == 4952) left_margin = 0;
	//		if (width == 3328 && (width -= 66)) left_margin = 34;
	//		if (width == 4936) left_margin = 4;
	//		if (!strcmp(model,"HS50EXR") ||
	//				!strcmp(model,"F900EXR")) {
	//			width += 2;
	//			left_margin = 0;
	//			filters = 0x16161616;
	//		}
	//		if (fuji_layout) raw_width *= is_raw;
	//		if (filters == 9)
	//			FORC(36) ((char *)xtrans)[c] =
	//					xtrans_abs[(c/6+top_margin) % 6][(c+left_margin) % 6];
	//	} else if (!strcmp(model,"KD-400Z")) {
	//		height = 1712;
	//		width  = 2312;
	//		raw_width = 2336;
	//		goto konica_400z;
	//	} else if (!strcmp(model,"KD-510Z")) {
	//		goto konica_510z;
	//	} else if (!strcasecmp(make,"Minolta")) {
	//		if (!load_raw && (maximum = 0xfff))
	//			load_raw = &CLASS unpacked_load_raw;
	//		if (!strncmp(model,"DiMAGE A",8)) {
	//			if (!strcmp(model,"DiMAGE A200"))
	//				filters = 0x49494949;
	//			tiff_bps = 12;
	//			load_raw = &CLASS packed_load_raw;
	//		} else if (!strncmp(model,"ALPHA",5) ||
	//				!strncmp(model,"DYNAX",5) ||
	//				!strncmp(model,"MAXXUM",6)) {
	//			sprintf (model+20, "DYNAX %-10s", model+6+(model[0]=='M'));
	//			adobe_coeff (make, model+20);
	//			load_raw = &CLASS packed_load_raw;
	//		} else if (!strncmp(model,"DiMAGE G",8)) {
	//			if (model[8] == '4') {
	//				height = 1716;
	//				width  = 2304;
	//			} else if (model[8] == '5') {
	//				konica_510z:
	//				height = 1956;
	//				width  = 2607;
	//				raw_width = 2624;
	//			} else if (model[8] == '6') {
	//				height = 2136;
	//				width  = 2848;
	//			}
	//			data_offset += 14;
	//			filters = 0x61616161;
	//			konica_400z:
	//			load_raw = &CLASS unpacked_load_raw;
	//			maximum = 0x3df;
	//			order = 0x4d4d;
	//		}
	//	} else if (!strcmp(model,"*ist D")) {
	//		load_raw = &CLASS unpacked_load_raw;
	//		data_error = -1;
	//	} else if (!strcmp(model,"*ist DS")) {
	//		height -= 2;
	//	} else if (!strcmp(make,"Samsung") && raw_width == 4704) {
	//		height -= top_margin = 8;
	//		width -= 2 * (left_margin = 8);
	//		load_flags = 32;
	//	} else if (!strcmp(make,"Samsung") && raw_height == 3714) {
	//		height -= top_margin = 18;
	//		left_margin = raw_width - (width = 5536);
	//		if (raw_width != 5600)
	//			left_margin = top_margin = 0;
	//		filters = 0x61616161;
	//		colors = 3;
	//	} else if (!strcmp(make,"Samsung") && raw_width == 5632) {
	//		order = 0x4949;
	//		height = 3694;
	//		top_margin = 2;
	//		width  = 5574 - (left_margin = 32 + tiff_bps);
	//		if (tiff_bps == 12) load_flags = 80;
	//	} else if (!strcmp(make,"Samsung") && raw_width == 5664) {
	//		height -= top_margin = 17;
	//		left_margin = 96;
	//		width = 5544;
	//		filters = 0x49494949;
	//	} else if (!strcmp(make,"Samsung") && raw_width == 6496) {
	//		filters = 0x61616161;
	//		black = 1 << (tiff_bps - 7);
	//	} else if (!strcmp(model,"EX1")) {
	//		order = 0x4949;
	//		height -= 20;
	//		top_margin = 2;
	//		if ((width -= 6) > 3682) {
	//			height -= 10;
	//			width  -= 46;
	//			top_margin = 8;
	//		}
	//	} else if (!strcmp(model,"WB2000")) {
	//		order = 0x4949;
	//		height -= 3;
	//		top_margin = 2;
	//		if ((width -= 10) > 3718) {
	//			height -= 28;
	//			width  -= 56;
	//			top_margin = 8;
	//		}
	//	} else if (strstr(model,"WB550")) {
	//		strcpy (model, "WB550");
	//	} else if (!strcmp(model,"EX2F")) {
	//		height = 3045;
	//		width  = 4070;
	//		top_margin = 3;
	//		order = 0x4949;
	//		filters = 0x49494949;
	//		load_raw = &CLASS unpacked_load_raw;
	//	} else if (!strcmp(model,"STV680 VGA")) {
	//		black = 16;
	//	} else if (!strcmp(model,"N95")) {
	//		height = raw_height - (top_margin = 2);
	//	} else if (!strcmp(model,"640x480")) {
	//		gamma_curve (0.45, 4.5, 1, 255);
	//	} else if (!strcmp(make,"Hasselblad")) {
	//		if (load_raw == &CLASS lossless_jpeg_load_raw)
	//			load_raw = &CLASS hasselblad_load_raw;
	//		if (raw_width == 7262) {
	//			height = 5444;
	//			width  = 7248;
	//			top_margin  = 4;
	//			left_margin = 7;
	//			filters = 0x61616161;
	//		} else if (raw_width == 7410 || raw_width == 8282) {
	//			height -= 84;
	//			width  -= 82;
	//			top_margin  = 4;
	//			left_margin = 41;
	//			filters = 0x61616161;
	//		} else if (raw_width == 9044) {
	//			height = 6716;
	//			width  = 8964;
	//			top_margin  = 8;
	//			left_margin = 40;
	//			black += load_flags = 256;
	//			maximum = 0x8101;
	//		} else if (raw_width == 4090) {
	//			strcpy (model, "V96C");
	//			height -= (top_margin = 6);
	//			width -= (left_margin = 3) + 7;
	//			filters = 0x61616161;
	//		}
	//		if (tiff_samples > 1) {
	//			is_raw = tiff_samples+1;
	//			if (!shot_select && !half_size) filters = 0;
	//		}
	//	} else if (!strcmp(make,"Sinar")) {
	//		if (!load_raw) load_raw = &CLASS unpacked_load_raw;
	//		if (is_raw > 1 && !shot_select && !half_size) filters = 0;
	//		maximum = 0x3fff;
	//	} else if (!strcmp(make,"Leaf")) {
	//		maximum = 0x3fff;
	//		fseek (ifp, data_offset, SEEK_SET);
	//		if (ljpeg_start (&jh, 1) && jh.bits == 15)
	//			maximum = 0x1fff;
	//		if (tiff_samples > 1) filters = 0;
	//		if (tiff_samples > 1 || tile_length < raw_height) {
	//			load_raw = &CLASS leaf_hdr_load_raw;
	//			raw_width = tile_width;
	//		}
	//		if ((width | height) == 2048) {
	//			if (tiff_samples == 1) {
	//				filters = 1;
	//				strcpy (cdesc, "RBTG");
	//				strcpy (model, "CatchLight");
	//				top_margin =  8; left_margin = 18; height = 2032; width = 2016;
	//			} else {
	//				strcpy (model, "DCB2");
	//				top_margin = 10; left_margin = 16; height = 2028; width = 2022;
	//			}
	//		} else if (width+height == 3144+2060) {
	//			if (!model[0]) strcpy (model, "Cantare");
	//			if (width > height) {
	//				top_margin = 6; left_margin = 32; height = 2048;  width = 3072;
	//				filters = 0x61616161;
	//			} else {
	//				left_margin = 6;  top_margin = 32;  width = 2048; height = 3072;
	//				filters = 0x16161616;
	//			}
	//			if (!cam_mul[0] || model[0] == 'V') filters = 0;
	//			else is_raw = tiff_samples;
	//		} else if (width == 2116) {
	//			strcpy (model, "Valeo 6");
	//			height -= 2 * (top_margin = 30);
	//			width -= 2 * (left_margin = 55);
	//			filters = 0x49494949;
	//		} else if (width == 3171) {
	//			strcpy (model, "Valeo 6");
	//			height -= 2 * (top_margin = 24);
	//			width -= 2 * (left_margin = 24);
	//			filters = 0x16161616;
	//		}
	//	} else if (!strcmp(make,"Leica") || !strcmp(make,"Panasonic")) {
	//		if ((flen - data_offset) / (raw_width*8/7) == raw_height)
	//			load_raw = &CLASS panasonic_load_raw;
	//		if (!load_raw) {
	//			load_raw = &CLASS unpacked_load_raw;
	//			load_flags = 4;
	//		}
	//		zero_is_bad = 1;
	//		if ((height += 12) > raw_height) height = raw_height;
	//		for (i=0; i < sizeof pana / sizeof *pana; i++)
	//			if (raw_width == pana[i][0] && raw_height == pana[i][1]) {
	//				left_margin = pana[i][2];
	//				top_margin = pana[i][3];
	//				width += pana[i][4];
	//				height += pana[i][5];
	//			}
	//		filters = 0x01010101 * (uchar) "\x94\x61\x49\x16"
	//												[((filters-1) ^ (left_margin & 1) ^ (top_margin << 1)) & 3];
	//	} else if (!strcmp(model,"C770UZ")) {
	//		height = 1718;
	//		width  = 2304;
	//		filters = 0x16161616;
	//		load_raw = &CLASS packed_load_raw;
	//		load_flags = 30;
	//	} else if (!strcmp(make,"Olympus")) {
	//		height += height & 1;
	//		if (exif_cfa) filters = exif_cfa;
	//		if (width == 4100) width -= 4;
	//		if (width == 4080) width -= 24;
	//		if (width == 9280) { width -= 6; height -= 6; }
	//		if (load_raw == &CLASS unpacked_load_raw)
	//			load_flags = 4;
	//		tiff_bps = 12;
	//		if (!strcmp(model,"E-300") ||
	//				!strcmp(model,"E-500")) {
	//			width -= 20;
	//			if (load_raw == &CLASS unpacked_load_raw) {
	//				maximum = 0xfc3;
	//				memset (cblack, 0, sizeof cblack);
	//			}
	//		} else if (!strcmp(model,"E-330")) {
	//			width -= 30;
	//			if (load_raw == &CLASS unpacked_load_raw)
	//				maximum = 0xf79;
	//		} else if (!strcmp(model,"SP550UZ")) {
	//			thumb_length = flen - (thumb_offset = 0xa39800);
	//			thumb_height = 480;
	//			thumb_width  = 640;
	//		}
	//	} else if (!strcmp(model,"N Digital")) {
	//		height = 2047;
	//		width  = 3072;
	//		filters = 0x61616161;
	//		data_offset = 0x1a00;
	//		load_raw = &CLASS packed_load_raw;
	//	} else if (!strcmp(model,"DSC-F828")) {
	//		width = 3288;
	//		left_margin = 5;
	//		mask[1][3] = -17;
	//		data_offset = 862144;
	//		load_raw = &CLASS sony_load_raw;
	//		filters = 0x9c9c9c9c;
	//		colors = 4;
	//		strcpy (cdesc, "RGBE");
	//	} else if (!strcmp(model,"DSC-V3")) {
	//		width = 3109;
	//		left_margin = 59;
	//		mask[0][1] = 9;
	//		data_offset = 787392;
	//		load_raw = &CLASS sony_load_raw;
	//	} else if (!strcmp(make,"Sony") && raw_width == 3984) {
	//		width = 3925;
	//		order = 0x4d4d;
	//	} else if (!strcmp(make,"Sony") && raw_width == 4288) {
	//		width -= 32;
	//	} else if (!strcmp(make,"Sony") && raw_width == 4928) {
	//		if (height < 3280) width -= 8;
	//	} else if (!strcmp(make,"Sony") && raw_width == 5504) {
	//		width -= height > 3664 ? 8 : 32;
	//	} else if (!strcmp(make,"Sony") && raw_width == 6048) {
	//		width -= 24;
	//		if (strstr(model,"RX1") || strstr(model,"A99"))
	//			width -= 6;
	//	} else if (!strcmp(make,"Sony") && raw_width == 7392) {
	//		width -= 30;
	//	} else if (!strcmp(model,"DSLR-A100")) {
	//		if (width == 3880) {
	//			height--;
	//			width = ++raw_width;
	//		} else {
	//			height -= 4;
	//			width  -= 4;
	//			order = 0x4d4d;
	//			load_flags = 2;
	//		}
	//		filters = 0x61616161;
	//	} else if (!strcmp(model,"DSLR-A350")) {
	//		height -= 4;
	//	} else if (!strcmp(model,"PIXL")) {
	//		height -= top_margin = 4;
	//		width -= left_margin = 32;
	//		gamma_curve (0, 7, 1, 255);
	//	} else if (!strcmp(model,"C603") || !strcmp(model,"C330")
	//			|| !strcmp(model,"12MP")) {
	//		order = 0x4949;
	//		if (filters && data_offset) {
	//			fseek (ifp, data_offset < 4096 ? 168 : 5252, SEEK_SET);
	//			read_shorts (curve, 256);
	//		} else gamma_curve (0, 3.875, 1, 255);
	//		load_raw  =  filters   ? &CLASS eight_bit_load_raw :
	//				strcmp(model,"C330") ? &CLASS kodak_c603_load_raw :
	//						&CLASS kodak_c330_load_raw;
	//						load_flags = tiff_bps > 16;
	//						tiff_bps = 8;
	//	} else if (!strncasecmp(model,"EasyShare",9)) {
	//		data_offset = data_offset < 0x15000 ? 0x15000 : 0x17000;
	//		load_raw = &CLASS packed_load_raw;
	//	} else if (!strcasecmp(make,"Kodak")) {
	//		if (filters == UINT_MAX) filters = 0x61616161;
	//		if (!strncmp(model,"NC2000",6) ||
	//				!strncmp(model,"EOSDCS",6) ||
	//				!strncmp(model,"DCS4",4)) {
	//			width -= 4;
	//			left_margin = 2;
	//			if (model[6] == ' ') model[6] = 0;
	//			if (!strcmp(model,"DCS460A")) goto bw;
	//		} else if (!strcmp(model,"DCS660M")) {
	//			black = 214;
	//			goto bw;
	//		} else if (!strcmp(model,"DCS760M")) {
	//			bw:   colors = 1;
	//			filters = 0;
	//		}
	//		if (!strcmp(model+4,"20X"))
	//			strcpy (cdesc, "MYCY");
	//		if (strstr(model,"DC25")) {
	//			strcpy (model, "DC25");
	//			data_offset = 15424;
	//		}
	//		if (!strncmp(model,"DC2",3)) {
	//			raw_height = 2 + (height = 242);
	//			if (flen < 100000) {
	//				raw_width = 256; width = 249;
	//				pixel_aspect = (4.0*height) / (3.0*width);
	//			} else {
	//				raw_width = 512; width = 501;
	//				pixel_aspect = (493.0*height) / (373.0*width);
	//			}
	//			top_margin = left_margin = 1;
	//			colors = 4;
	//			filters = 0x8d8d8d8d;
	//			simple_coeff(1);
	//			pre_mul[1] = 1.179;
	//			pre_mul[2] = 1.209;
	//			pre_mul[3] = 1.036;
	//			load_raw = &CLASS eight_bit_load_raw;
	//		} else if (!strcmp(model,"40")) {
	//			strcpy (model, "DC40");
	//			height = 512;
	//			width  = 768;
	//			data_offset = 1152;
	//			load_raw = &CLASS kodak_radc_load_raw;
	//		} else if (strstr(model,"DC50")) {
	//			strcpy (model, "DC50");
	//			height = 512;
	//			width  = 768;
	//			data_offset = 19712;
	//			load_raw = &CLASS kodak_radc_load_raw;
	//		} else if (strstr(model,"DC120")) {
	//			strcpy (model, "DC120");
	//			height = 976;
	//			width  = 848;
	//			pixel_aspect = height/0.75/width;
	//			load_raw = tiff_compress == 7 ?
	//					&CLASS kodak_jpeg_load_raw : &CLASS kodak_dc120_load_raw;
	//		} else if (!strcmp(model,"DCS200")) {
	//			thumb_height = 128;
	//			thumb_width  = 192;
	//			thumb_offset = 6144;
	//			thumb_misc   = 360;
	//			write_thumb = &CLASS layer_thumb;
	//			black = 17;
	//		}
	//	} else if (!strcmp(model,"Fotoman Pixtura")) {
	//		height = 512;
	//		width  = 768;
	//		data_offset = 3632;
	//		load_raw = &CLASS kodak_radc_load_raw;
	//		filters = 0x61616161;
	//		simple_coeff(2);
	//	} else if (!strncmp(model,"QuickTake",9)) {
	//		if (head[5]) strcpy (model+10, "200");
	//		fseek (ifp, 544, SEEK_SET);
	//		height = get2();
	//		width  = get2();
	//		data_offset = (get4(),get2()) == 30 ? 738:736;
	//		if (height > width) {
	//			SWAP(height,width);
	//			fseek (ifp, data_offset-6, SEEK_SET);
	//			flip = ~get2() & 3 ? 5:6;
	//		}
	//		filters = 0x61616161;
	//	} else if (!strcmp(make,"Rollei") && !load_raw) {
	//		switch (raw_width) {
	//		case 1316:
	//			height = 1030;
	//			width  = 1300;
	//			top_margin  = 1;
	//			left_margin = 6;
	//			break;
	//		case 2568:
	//			height = 1960;
	//			width  = 2560;
	//			top_margin  = 2;
	//			left_margin = 8;
	//		}
	//		filters = 0x16161616;
	//		load_raw = &CLASS rollei_load_raw;
	//	}

	if (!model[0]) {
		sprintf (model, "%dx%d", width, height);
	}
	if (filters == UINT_MAX) filters = 0x94949494;
	if (thumb_offset && !thumb_height) {
		cout << ERROR_LINE << "Not implemented yet." << endl; exit(0);
		//		fseek (ifp, thumb_offset, SEEK_SET);
		//		if (ljpeg_start (&jh, 1)) {
		//			thumb_width  = jh.wide;
		//			thumb_height = jh.high;
		//		}
	}

	//dng_skip:
	if ((use_camera_matrix & (use_camera_wb || dng_version))
			&& cmatrix[0][0] > 0.125) {
		cout << ERROR_LINE << "Not implemented yet." << endl;
		memcpy (rgb_cam, cmatrix, sizeof cmatrix);
		raw_color = 0;
	}
	if (raw_color)  {
		DBG_MESSAGE("Not implemented yet. for rgb_cam, xyz_cam");
		//adobe_coeff (make, model);
	}
	//		if (load_raw == &CLASS kodak_radc_load_raw)
	//			if (raw_color) adobe_coeff ("Apple","Quicktake");

	if (fuji_width) {
		cout << ERROR_LINE << "Not implemented yet." << endl;
		//			fuji_width = width >> !fuji_layout;
		//			filters = fuji_width & 1 ? 0x94949494 : 0x49494949;
		//			width = (height >> fuji_layout) + fuji_width;
		//			height = width - 1;
		//			pixel_aspect = 1;
	} else {
		if (raw_height < height) raw_height = height;
		if (raw_width  < width ) raw_width  = width;
	}
	if (!tiff_bps) { tiff_bps = 12; };
	if (!maximum) { maximum = (1 << tiff_bps) - 1; }
	if (!load_raw || height < 22 || width < 22 ||
			tiff_bps > 16 || tiff_samples > 6 || colors > 4) {
		is_raw = 0;
	}

#ifdef NO_JASPER
	//cerr << ERROR_LINE << "error" << endl; exit(0);
	//	if (load_raw == &CLASS redcine_load_raw) {
	//		fprintf (stderr,_("%s: You must link dcraw with %s!!\n"),
	//				ifname, "libjasper");
	//		is_raw = 0;
	//	}
#endif
#ifdef NO_JPEG
	//cerr << ERROR_LINE << "error" << endl; exit(0);
	//if (load_raw == &CLASS kodak_jpeg_load_raw ||
	//			load_raw == &CLASS lossy_dng_load_raw) {
	//		fprintf (stderr,_("%s: You must link dcraw with %s!!\n"),
	//				ifname, "libjpeg");
	//		is_raw = 0;
	//}
#endif
	if (!cdesc[0]) {
		strcpy (cdesc, colors == 3 ? "RGBG":"GMCY");
	}
	if (!raw_height)  {
		cout << ERROR_LINE << "Not implemented yet." << endl;
		raw_height = height;
	}
	if (!raw_width ) {
		cout << ERROR_LINE << "Not implemented yet." << endl;
		raw_width  = width;
	}
	if (filters > 999 && colors == 3) {
		filters |= ((filters >> 2 & 0x22222222) |
				(filters << 2 & 0x88888888)) & filters << 1;
	}
	//notraw:
	if (dcraw_flip == UINT_MAX) dcraw_flip = tiff_flip;
	if (dcraw_flip == UINT_MAX) dcraw_flip = 0;
	//cerr << ERROR_LINE << "error " << make << " " << model << " " << is_raw << " " << raw_width << "x" << raw_height << " " << tiff_bps << endl; exit(0);
}



//identity()
//	parse_tiff()
//		ljpeg_start()
//		parse_tiff_ifd()
//		parse_exif()
//		parse_gpsdata()
//	apply_tiff() <- nikon_load_rawがセットされる

//				int CLASS main (int argc, const char **argv)
NEF readNEF0(string file_name)
{
	DBG_MESSAGE("start");
	NEF nef;

	int arg, status=0, quality, i, c;
	int identify_only=0,exifsummary_only=0,exifsummary_header=0;
	int user_qual=-1, user_black=-1, user_sat=-1, user_flip=-1;
	int use_fuji_rotate=1, write_to_stdout=0, read_from_stdin=0;
	const char *sp, *write_ext;
	char opm, opt, *ofname, *cp;
	struct utimbuf ut;

	//	int document_mode = 0; //half_size=0, four_color_rgb=0, highlight=0;
	//	int output_tiff = 0; //output_color=1, output_bps=8, med_passes=0;

#ifndef LOCALTIME
	putenv ((char *) "TZ=UTC");
#endif
#ifdef LOCALEDIR
	setlocale (LC_CTYPE, "");
	setlocale (LC_MESSAGES, "");
	bindtextdomain ("dcraw", LOCALEDIR);
	textdomain ("dcraw");
#endif

	//copied from global scope
	//const char *ifname = file_name.c_str();
	int document_mode = 1;
	int output_tiff = 1;

	status = 1;
	//	raw_image = NULL;
	//	image = NULL;
	//	oprof = NULL;
	//	meta_data = ofname = NULL;
	//ofp = stdout;

	if (!(ifp = fopen (file_name.c_str(), "rb"))) {
		cout << ERROR_LINE << file_name<< endl;
		exit(0);
	}
	status = (identify(),!is_raw);
	if (user_flip >= 0)
		dcraw_flip = user_flip;
	switch ((dcraw_flip+3600) % 360) {
	case 270:  dcraw_flip = 5;  break;
	case 180:  dcraw_flip = 3;  break;
	case  90:  dcraw_flip = 6;
	}
	//	write_fun = &CLASS write_ppm_tiff;
	//	if (load_raw == &CLASS kodak_ycbcr_load_raw) {
	//		height += height & 1;
	//		width  += width  & 1;
	//	}

	//TODO
	char timestr[256];
	strftime(timestr, 255, "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
	char exposure[16];
	if(shutter > 0 && shutter < 1) { sprintf(exposure, "%6.0f", 1 / shutter); }
	else { sprintf(exposure, "%6.3fs", shutter); }
	char camera[64];
	sprintf(camera, "\"%s\"", model);
	ushort *raw_image = NULL;

	nef.fileName = file_name;
	nef.date = timestr;
	nef.cameraName = camera;
	nef.ISO = iso_speed;
	nef.exposure = exposure;
	nef.aperture = aperture;
	nef.focalLength = focal_len;

	if(exifsummary_header) {
		nef.printInfoHeader();
	}
	if(exifsummary_only) {
		DBG_MESSAGE("exifsummary_only");
		nef.printInfo();
		exit(0);
	}
	bool verbose = false;
	if (identify_only && verbose && make[0]) {
		cerr << ERROR_LINE << "error" << endl; exit(0);
		//		DBG_MESSAGE("identify_only && verbose && make[0]");
		//		printf (_("\nFilename: %s\n"), ifname);
		//		printf (_("Timestamp: %s"), ctime(&timestamp));
		//		printf (_("Camera: %s %s\n"), make, model);
		//		if (artist[0])
		//			printf (_("Owner: %s\n"), artist);
		//		if (dng_version) {
		//			printf (_("DNG Version: "));
		//			for (i=24; i >= 0; i -= 8)
		//				printf ("%d%c", dng_version >> i & 255, i ? '.':'\n');
		//		}
		//		printf (_("ISO speed: %d\n"), (int) iso_speed);
		//		printf (_("Shutter: "));
		//		if (shutter > 0 && shutter < 1)
		//			shutter = (printf ("1/"), 1 / shutter);
		//		printf (_("%0.1f sec\n"), shutter);
		//		printf (_("Aperture: f/%0.1f\n"), aperture);
		//		printf (_("Focal length: %0.1f mm\n"), focal_len);
		//		printf (_("Embedded ICC profile: %s\n"), profile_length ? _("yes"):_("no"));
		//		printf (_("Number of raw images: %d\n"), is_raw);
		//		if (pixel_aspect != 1)
		//			printf (_("Pixel Aspect Ratio: %0.6f\n"), pixel_aspect);
		//		if (thumb_offset)
		//			printf (_("Thumb size:  %4d x %d\n"), thumb_width, thumb_height);
		//		printf (_("Full size:   %4d x %d\n"), raw_width, raw_height);
	}
	else if (!is_raw) {
		cerr << ERROR_LINE << "error" << endl; exit(0);
		fprintf (stderr,_("Cannot decode file %s\n"), file_name.c_str());
	}
	//if (!is_raw) goto next;

	//	shrink = filters && (half_size || (!identify_only &&
	//			(threshold || aber[0] != 1 || aber[2] != 1)));
	//	iheight = (height + shrink) >> shrink;
	//	iwidth  = (width  + shrink) >> shrink;
	if (identify_only) {
		cerr << ERROR_LINE << "error" << endl; exit(0);
		//		DBG_MESSAGE("identify_only");
		//		if (verbose) {
		//			if (document_mode == 3) {
		//				top_margin = left_margin = fuji_width = 0;
		//				height = raw_height;
		//				width  = raw_width;
		//			}
		//			iheight = (height + shrink) >> shrink;
		//			iwidth  = (width  + shrink) >> shrink;
		//			if (use_fuji_rotate) {
		//				if (fuji_width) {
		//					fuji_width = (fuji_width - 1 + shrink) >> shrink;
		//					iwidth = fuji_width / sqrt(0.5);
		//					iheight = (iheight - fuji_width) / sqrt(0.5);
		//				} else {
		//					if (pixel_aspect < 1) iheight = iheight / pixel_aspect + 0.5;
		//					if (pixel_aspect > 1) iwidth  = iwidth  * pixel_aspect + 0.5;
		//				}
		//			}
		//			if (flip & 4)
		//				SWAP(iheight,iwidth);
		//			printf (_("Image size:  %4d x %d\n"), width, height);
		//			printf (_("Output size: %4d x %d\n"), iwidth, iheight);
		//			printf (_("Raw colors: %d"), colors);
		//			if (filters) {
		//				int fhigh = 2, fwide = 2;
		//				if ((filters ^ (filters >>  8)) & 0xff)   fhigh = 4;
		//				if ((filters ^ (filters >> 16)) & 0xffff) fhigh = 8;
		//				if (filters == 1) fhigh = fwide = 16;
		//				if (filters == 9) fhigh = fwide = 6;
		//				printf (_("\nFilter pattern: "));
		//				for (i=0; i < fhigh; i++)
		//					for (c = i && putchar('/') && 0; c < fwide; c++)
		//						putchar (cdesc[fcol(i,c)]);
		//			}
		//			printf (_("\nDaylight multipliers:"));
		//			FORCC printf (" %f", pre_mul[c]);
		//			if (cam_mul[0] > 0) {
		//				printf (_("\nCamera multipliers:"));
		//				FORC4 printf (" %f", cam_mul[c]);
		//			}
		//			putchar ('\n');
		//		} else
		//			printf (_("%s is a %s %s image.\n"), ifname, make, model);
		//		next:
		//		fclose(ifp);
		//		continue;
	}

	if (meta_length) {
		cerr << ERROR_LINE << "error" << endl; exit(0);
		//		DBG_MESSAGE("meta_length");
		//		meta_data = (char *) malloc (meta_length);
		//		merror (meta_data, "main()");
	}
	if (filters || colors == 1) {
		DBG_MESSAGE("filters || colors == 1 raw_image is not used.");
//		cout << ERROR_LINE << "Not implemented yet. " << raw_height+7 << " " << raw_width*2 << " " << sizeof(ushort) << endl;
//		//raw_image = (ushort *) calloc ((raw_height+7), raw_width*2);
//		//raw_image = (ushort *) calloc ((raw_height+7) * raw_width, sizeof(ushort));
//		merror (raw_image, "main()");
	} else {
		cerr << ERROR_LINE << "error" << endl; exit(0);
		//		DBG_MESSAGE("else");
		//		image = (ushort (*)[4]) calloc (iheight, iwidth*sizeof *image);
		//		merror (image, "main()");
	}

	if (verbose) {
	//		fprintf (stderr,_("Loading %s %s image from %s ...\n"),
	//				make, model, ifname);
	}
	if (shot_select >= is_raw) {
		cout << ERROR_LINE << "Not implemented yet." << endl;
		fprintf (stderr, _("%s: \"-s %d\" requests a nonexistent image!\n"),
					file_name.c_str(), shot_select);
	}
	fseeko (ifp, data_offset, SEEK_SET);

	DBG_MESSAGE("(*load_raw)();");
	(*load_raw)(nef);
	DBG_MESSAGE("after (*load_raw)();");
	//cout << raw_image << endl;
	//free(raw_image); raw_image = NULL;
	DBG_MESSAGE("");
	fclose(ifp);
	//	DBG_MESSAGE("");
	//
	//
	//	if (ofp != stdout) fclose(ofp);
	//	DBG_MESSAGE("");
	//	//if (meta_data) free (meta_data);
	//	if (ofname) { free (ofname); ofname = NULL; }
	//	DBG_MESSAGE("");
	//	if (oprof) free (oprof);
	//	DBG_MESSAGE("");
	//	//if (image) free (image);
	//	if (multi_out) {
	//		if (++shot_select < is_raw) arg--;
	//		else shot_select = 0;
	//	}
	//	DBG_MESSAGE("before return(nef)");

	nef.status = status;
	return(nef);
}

} // end of dcraw
