/*
 * complex_proc.c
 *
 *  Created on: Mar 7, 2011
 *      Author: wibble
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define _FILE_OFFSET_BITS 64

#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdbool.h>
#include <fftw3.h>

#include "complex_proc.h"

#define PI 3.14159265
#define N_CHAN 2
#define HLINES 512
#define VLINES 650
#define MAX_AVG 32
#define PLIMIT 255.0
#define BGCN 210.0

static char head_strings[2][2][33] =
        {
                { // Normal byte order
                        "aDtromtu hoCllge eaMtsreR DxPS  \0",
                        "aDtromtu hoCllge elSva eR DxPS  \0"
                },
                { // Swapped byte order (used with -E)
		  // (I guess short ints are read in with bytes swapped compared to disk order?)
                        "Dartmouth College Master RxDSP  \0",
                        "Dartmouth College Slave  RxDSP  \0"
                }
        };

static bool running = true;

static inline short int endswp(short int x) {
	return((short signed int)( (((short unsigned int) x)>>8) | (((short unsigned int) x)<<8) ));
}

static short int *samples;
static fftw_complex *fft_input;
static fftw_complex *fft_output;
static fftw_plan fft_plan;
static double **powers;
static double *freqs, *window;
static struct images *im;
static double df;
static int mapsize;
static char *imap;//, *tmap;

int main (int argc, char **argv) {

	float r;
	struct f2i_coef co;
	int hlines, vlines, average, n_chan, ind, atotal, shift, ret;
	short int *hptr;
	char *dptr = NULL;

	char file_str[1024], lvlfile[1024], hf2_runfile[1024], onecal[50];
	struct gray_vals gray;
	struct timeval then;
	struct header_info header, head_temp;

	double calary[4096], datai;
	struct cp_opts o;

	FILE *tempfile, *specfile, **imagfile, *calfile;

	signal(SIGINT, do_depart);

	init_opt(&o);
	parse_opt(&o, argc, argv);

	if ((o.freq_bw > 0) && (o.freq_cent > 0)) {
		o.freq_min = floor(o.freq_cent-o.freq_bw/2.0);
		o.freq_max = ceil(o.freq_cent+o.freq_bw/2.0);
	}

	if (o.agccal) {
		printf("Loading cal array...");
		calfile = fopen(o.agcfile,"r");
		if (calfile == NULL) {
			printf("failed!.\n"); return(1);
		}
		for (int ical = 0; ical < 4096; ical++) {
			ret = fscanf(calfile,"%s",onecal);
			calary[ical] = strtod(onecal, NULL);
		}
		fclose(calfile);
		printf("done.\n");
	}

	hlines = HLINES;
	vlines = VLINES;
	n_chan = N_CHAN;
	average = 1;

	printf("Initializing...");fflush(stdout);

	gettimeofday(&then, NULL);

	memset(&header, 0, sizeof(struct header_info));
	/*	infile = fopen(o.infile, "r");
	if (infile == NULL) {
		printf("failed to open %s.\n", o.infile); return(1);
	}*/
	sprintf(file_str, "%s/test.data", o.tmpdir);
	specfile = fopen(file_str, "w");
	if (specfile == NULL) {
		printf("failed to open %s.\n", file_str); return(1);
	}
	int specfd = fileno(specfile);
	imagfile = malloc(N_CHAN * sizeof(FILE *));
	sprintf(hf2_runfile, "%s/hf2_display_running", o.tmpdir);
	sprintf(lvlfile, "%s/levels.grayscale", o.tmpdir);

	for (int chan = 0; chan < N_CHAN; chan++) {
		sprintf(file_str, "%s/test.image%1i", o.tmpdir, chan+1);
		imagfile[chan] = fopen(file_str, "w");
		if (imagfile[chan] == NULL) {
			printf("failed to open %s.\n", file_str); return(1);
		}
	}

	printf("done.\n");

	initialize_dynvar(hlines, vlines, average, n_chan, &o);

	//	tmap = mmap_file("/tmp/rawrtdout", mapsize, O_RDWR|O_TRUNC);
	long long unsigned int count = 0;
	// Main loop
	while (running) {
		do { // Wait for rtd data file update
			if (!running) break;
			usleep(o.granularity);
			memmove(&head_temp, imap, sizeof(struct header_info));
			if (head_temp.hkey == 0xF00FABBA) header = head_temp;
		} while ((then.tv_sec == header.start_timeval.tv_sec) && (then.tv_usec == header.start_timeval.tv_usec));

		then = header.start_timeval;
		average = header.averages;
		n_chan = header.num_channels;

		//        printf("fail");fflush(stdout);

		dptr = realloc(dptr, 2*header.num_read);
		//        printf("searching %li bytes\n",2*header.num_read);
		memmove(dptr, &imap[100], 2*header.num_read);

		initialize_dynvar(hlines, vlines, average, n_chan, &o);

		if ((tempfile = fopen(lvlfile, "r")) == NULL) {
			gray.min = 40; gray.max = 140.0;
		} else {
			ret = fscanf(tempfile, "%d %d", &gray.min, &gray.max);
			fclose(tempfile);
		}

		co = rescale_images(im, gray, n_chan);

		atotal = hlines * average;

		for (int chan = 0; chan < n_chan; chan++) {
			hptr = memmem(dptr, 2*header.num_read, head_strings[o.endian][chan], 32); // Find channel #chan's header
			if (hptr == NULL) {
				printf("Failed to find channel %i's header!\n", chan+1);
				continue;
			}

			int lsb_offset = 0; // Find first non-zero LSB
			for (int i = 22; i < 200 && !lsb_offset; i++) {
				if (hptr[i]&1) lsb_offset = i;
			}
			
			//			printf("lsb_offset: %i\n", lsb_offset); fflush(stdout);
			lsb_offset = 26; // Fudge.  Why was this lsb search in here?!

			samples = &hptr[lsb_offset];

			//			short int *reals = malloc(512*sizeof(short int));
			//			short int *compl = malloc(512*sizeof(short int));
			for (int i = 0; i < atotal; i++) {

				if (o.endian) {
					if (o.verbose) printf("Endian switch.\n");fflush(stdout);
					fft_input[i] =  ((double) endswp(samples[2*i]));
					fft_input[i] += ((double) endswp(samples[2*i+1]))*I;
				}
				else {
					if (o.verbose) printf("No endian switch.\n");fflush(stdout);
					fft_input[i] =  ((double) samples[2*i]);
					fft_input[i] += ((double) samples[2*i+1])*I;
				}
				//				reals[i] = (short int) creal(fft_input[i]);
				//				compl[i] = (short int) cimag(fft_input[i]);
				fft_input[i] *= window[i];

				//				fft_input[chan][i] = ((double) samples[shift+2*i]) + I*((double) samples[shift+2*i+1]);
			}

			//			memmove(tmap, reals, 512*sizeof(short int));
			//			memmove(tmap+1024*sizeof(short int), compl, 512*sizeof(short int));
			//			memmove(tmap, window, 512*sizeof(double));

			fftw_execute(fft_plan);

			for (int i = 0; i < hlines; i++) {
				shift = average*i;
				datai = 0;
				for (int z = 0; z < average; z++) {
					ind = shift+z;
					ind = ind < atotal/2 ? ind+atotal/2 : ind-atotal/2;
					datai += 2*pow( cabs(fft_output[ind]) ,2);
				}

				powers[chan][i] = 10 * log10( datai/average );
				powers[chan][i] -= 0;

				// Shift images over one vertical line
				ind = (hlines-1-i)*vlines;
				memmove(&im[chan].fl[ind], &im[chan].fl[ind+1], (vlines-1)*sizeof(float));
				memmove(&im[chan].in[ind], &im[chan].in[ind+1], (vlines-1)*sizeof(char));

				// Insert the new line, swap high and low frequencies (DFT jiggerypokery)
				r = (float) powers[chan][i];

				im[chan].fl[ind+(vlines-1)] = r;
				im[chan].in[ind+(vlines-1)] = f2iImage(r, &co);
			}
		} // for(chan)

		/*
		 * If the hf2 run file exists, we write; if not, why bother?
		 */
		//		if ((tempfile = fopen(hf2_runfile,"r")) != NULL) {
		//		fclose(tempfile);
		//	printf("write");fflush(stdout);

		rewind(specfile);
		ret = ftruncate(specfd, 0);
		for (int i = 0; i < hlines; i++) {
			fprintf(specfile, "%.0lf %.2lf %.2lf %.2lf %.2lf\n", freqs[i],
					powers[0%n_chan][i], powers[1%n_chan][i], powers[2%n_chan][i], powers[3%n_chan][i]);
		}
		fflush(specfile);

		for (int chan = 0; chan < n_chan; chan++) {
			rewind(imagfile[chan]);
			fprintf(imagfile[chan], "P5\n%i %i\n%i\n", vlines, hlines, (int) lrint(floor(PLIMIT)));
			ret = fwrite(im[chan].in, sizeof(unsigned char), hlines*vlines, imagfile[chan]);
			fflush(imagfile[chan]);
		}
		//} // if(write)

		count++;
		usleep(o.granularity);

	} // while(running)

	printf("done.\n");

	return(0);
}

unsigned char f2iImage(float fI, struct f2i_coef *coef) {
	/*
	 * Takes a float and scaling coefficients, returns scaled integer
	 */
	float v;

	v = coef->a + fI*coef->b + 0.5;
	if (v < 0) v = 0;
	if (v > PLIMIT) v = PLIMIT;

	return PLIMIT-( (unsigned char) lrint(v));
}

struct f2i_coef rescale_images(struct images *im, struct gray_vals new, int n_chan) {
	/*
	 * Tests if
	 * Runs through entire image, rebuilding the integer array from
	 * the (unscaled) float array, adjusting for overflows.
	 */
	static struct gray_vals *gray;
	struct f2i_coef co;

	if (gray == NULL) {
		gray = malloc(sizeof(struct gray_vals));
		*gray = new;
	}

	co.b = PLIMIT/(new.max-new.min);
	co.a = -co.b*new.min;

	if ((gray->min != new.min) || (gray->max != new.max)) {

		*gray = new;

		printf("Rescaling Images...\n");fflush(stdout);
		for (int chan = 0; chan < n_chan; chan++) {
			for (int i = 0; i < HLINES; i++) {
				int ind = i*VLINES;
				for (int j = 0; j < VLINES; j++) {
					im[chan].in[ind+j] = f2iImage(im[chan].fl[ind+j], &co);
				}
			}
		}
	}

	return(co);
}

void initialize_dynvar(int hlines, int vlines, int average, int n_chan, struct cp_opts *o) {
	/* initialize dynamic variables */

	static bool first = true;
	static int s_hl = 0, s_vl = 0, s_av = 0, s_nc = 0;
	bool total = false;
	if ((s_hl != hlines) || (s_vl != vlines) || (s_av != average) || (s_nc != n_chan)) {
		/*
		 * total: did we just change the number of averages,
		 * or is this a complete change of image specs?
		 */
		total = ((s_hl != hlines) || (s_vl != vlines) || (s_nc != n_chan)) ? true : false;
		if (first) {
			printf("Initializiing memory");
			first = false;
		}
		else { // Deallocate old arrays
			printf("Reinitializiing memory");

			if (total) {
				printf(" completely");
				for (int chan = 0; chan < s_nc; chan++) {
					free(im[chan].fl); free(im[chan].in); free(powers[chan]);
				}
				free(im);
			}
			munmap(imap, mapsize); free(samples);
			fftw_free(fft_input); fftw_free(fft_output); fftw_destroy_plan(fft_plan);
			free(freqs); free(window);
		}
		s_hl = hlines; s_vl = vlines; s_av = average; s_nc = n_chan;

		mapsize = o->filesize;
		imap = mmap_file(o->infile, mapsize, O_RDWR);

		if (total) {
			powers = malloc(s_nc * sizeof(double *));
			im = malloc(s_nc * sizeof(struct images));
		}

		for (int chan = 0; chan < s_nc; chan++) {
			fft_input = fftw_malloc(s_hl * s_av * sizeof(fftw_complex));
			fft_output = fftw_malloc(s_hl * s_av * sizeof(fftw_complex));
			fft_plan = fftw_plan_dft_1d(s_hl * s_av, fft_input, fft_output, FFTW_FORWARD, FFTW_MEASURE|FFTW_DESTROY_INPUT);

			if (total) {
				powers[chan] = malloc(s_hl * sizeof(double));
				im[chan].fl = malloc(s_hl * s_vl * sizeof(float));
				im[chan].in = malloc(s_hl * s_vl * sizeof(char));

				for (int j = 0; j < s_hl; j++) {
					powers[chan][j] = BGCN;
					int ind = j*s_vl;

					for (int k = 0; k < s_vl; k++) {
						im[chan].fl[ind+k] = BGCN;
						im[chan].in[ind+k] = BGCN;
					} // VLINES
				} // HLINES
			} // if(total)
		} // channels

		df = (o->freq_max-o->freq_min)/((double) s_hl);
		freqs = malloc(s_hl * sizeof(double));
		for (int i = 0; i < s_hl; i++) {
			freqs[i] = (o->freq_min + i*df);
		}

		window = malloc(s_hl * s_av * sizeof(double));
		for (int i = 0; i < s_hl * s_av; i++) {
			window[i] = 0.5-0.5*cos(2*PI*i/((s_hl * s_av)-1));
		}

		printf("...done.\n");fflush(stdout);
	} // if(changes)
} /* initialize_dynvar() */

static void do_depart(int signum) {
	running = false;
	fprintf(stderr, "\nStopping...");

	return;
}

int parse_opt(struct cp_opts *options, int argc, char **argv) {
	int c;

	while (-1 != (c = getopt(argc, argv, "m:f:F:c:C:B:g:s:Evh"))) {
		switch (c) {
		case 'm':
			options->infile = optarg;
			break;
		case 'f':
			options->freq_min = strtod(optarg, NULL);
			break;
		case 'F':
			options->freq_max = strtod(optarg, NULL);
			break;
		case 'C':
			options->freq_cent = strtod(optarg, NULL);
			break;
		case 'B':
			options->freq_bw = strtod(optarg, NULL);
			break;
		case 'c':
			options->agcfile = optarg;
			options->agccal = true;
			break;
		case 'g':
			options->granularity = strtoul(optarg, NULL, 0);
			break;
		case 's':
			options->filesize = strtoul(optarg, NULL, 0);
			break;
		case 'E':
			options->endian = !options->endian;
			break;
		case 't':
			options->tmpdir = optarg;
			break;
		case 'v':
			options->verbose = true;
			break;
		case 'h':
		default:
			printf("\ncprtd: Process data for hf2_display.\n\n Options:\n");
			printf("\t-m <#>\tReal-time display monitor file [Default: %s].\n", DEF_INFILE);
			printf("\t-f <#>\tLowest frequency in band [%i].\n", DEF_FREQ_MIN);
			printf("\t-F <#>\tHighest frequency in band [%i].\n", DEF_FREQ_MAX);
			printf("\t-c <s>\tAGC Calibration levels file [none],\n");
			printf("\t\tprovide to enable AGC of channel 1 on channel 3 output.\n");
			printf("\t-g <#>\tSet process granularity (in us) [%i].\n", DEF_GRAN);
			printf("\t-s <#>\tSet size of rtd data file (bytes) [%i].\n", DEF_FILESIZE);
			printf("\t-E\tSwap endianness of header search [%i].\n",DEF_ENDIAN);
			printf("\t-t <s>\tSet temporary directory [%s].\n", DEF_TMPDIR);
			printf("\t-v Be verbose.\n");
			printf("\t-h Display this message.\n\n");
			exit(1);
		}

	}

	return argc;
}

void init_opt(struct cp_opts *o) {
	memset(o, 0, sizeof(struct cp_opts));
	o->infile = DEF_INFILE;
	o->freq_min = DEF_FREQ_MIN;
	o->freq_max = DEF_FREQ_MAX;
	o->freq_cent = 0;
	o->freq_bw = 0;
	o->agccal = DEF_AGCCAL;
	o->agcfile = DEF_AGCFILE;
	o->granularity = DEF_GRAN;
	o->endian = DEF_ENDIAN;
	o->tmpdir = DEF_TMPDIR;
	o->filesize = DEF_FILESIZE;

	o->verbose = false;
}

void * mmap_file(char *name, int size, int flags) {
	int tfd, mflags = 0, ret;
	struct stat sb;
	char *p, *zeroes;

	if ((flags & O_RDONLY) | (flags & O_RDWR)) mflags |= PROT_READ;
	if ((flags & O_WRONLY) | (flags & O_RDWR)) mflags |= PROT_WRITE;
	flags |= O_CREAT;

	tfd = open(name, flags,
			S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	if (tfd == -1) {
		printe("Failed to open mmap file.", tfd); return(NULL);
	}
	if ((fstat(tfd, &sb) == -1) || (!S_ISREG(sb.st_mode))) {
		printe("Improper mmap target file.", sb.st_mode); return(NULL);
	}
	if (flags & O_TRUNC) {
		zeroes = malloc(size);
		memset(zeroes, 0, size);
		ret = write(tfd, zeroes, size);
		free(zeroes);
	}

	p = mmap(0, size, mflags, MAP_SHARED, tfd, 0);
	if (p == MAP_FAILED) {
		printe("mmap() of file failed.", 6); return(NULL);
	}

	ret = close(tfd);

	return(p);
}

void printe(char *format, ...) {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, strcat(format,"  Error Code: %i.\n"), args);
	va_end(args);
}
