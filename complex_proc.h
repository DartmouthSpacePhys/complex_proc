/*
 * complex_proc.h
 *
 *  Created on: Mar 11, 2011
 *      Author: wibble
 */

#ifndef COMPLEX_PROC_H_
#define COMPLEX_PROC_H_

#include <stdbool.h>

#define DEF_INFILE "/tmp/rtd/rtd.data"
#define DEF_FREQ_MIN 1283
#define DEF_FREQ_MAX 1617
#define DEF_AGCCAL false
#define DEF_AGCFILE ""
#define DEF_GRAN 5000
#define DEF_FILESIZE 262244
#define DEF_ENDIAN false
#define DEF_TMPDIR "/tmp/rtd"
#define DEF_PREFIX "test"

struct gray_vals {
	int min;
	int max;
};

struct f2i_coef {
	float a, b;
};

struct images {
	float *fl;
	unsigned char *in;
};

struct cp_opts {
	char *infile;
	double freq_min;
	double freq_max;
	double freq_cent;
	double freq_bw;
	bool agccal;
	char *agcfile;
	int granularity;
    size_t filesize;
	char *tmpdir;
        char *prefix;
	bool endian;

	bool verbose;
};

struct header_info {
	int hkey;
	char site_id[12];
	int num_channels;
	char channel_flags;
	unsigned int num_samples;
	unsigned int num_read;
	unsigned int averages;
	float sample_frequency;
	float time_between_acquisitions;
	int byte_packing;
	time_t start_time;
	struct timeval start_timeval;
	float code_version;
};

unsigned char f2iImage(float, struct f2i_coef *);
struct f2i_coef rescale_images(struct images *, struct gray_vals, int);
void initialize_dynvar(int, int, int, int, struct cp_opts *);
static void do_depart(int);
void * mmap_file(char *, int, int);
void init_opt(struct cp_opts *);
int parse_opt(struct cp_opts *, int, char **);
void printe(char *format, ...);

#endif /* COMPLEX_PROC_H_ */
