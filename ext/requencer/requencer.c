/** @file requencer.c 
 *
 * \brief Requencer Ruby C extension.
 *
 * This file contains the C sample rendering engine used by Requencer.
 */

#include <ruby.h>
#include <stdint.h>
#include <lame/lame.h>

#define MAX(a,b) ((a)>(b)?(a):(b)) ///< Compute maximum
#define MIN(a,b) ((a)<(b)?(a):(b)) ///< Compute minimum

#include <stdio.h>
#include <sys/time.h>

#ifdef DEBUG
	#define BENCHMARK(milestone) (benchmark(milestone))		///< Benchmark macro
#else
	#define BENCHMARK(milestone)							///< Benchmark macro
#endif

/**
 * \brief Benchmarking function.
 * 
 * Prints time elapsed since both last and first call of this function.
 * \note Do not call this function directly. Use the BENCHMARK macro instead.
 *
 * @param milestone A string representing the name of the current event
 */
static void benchmark(const char *milestone) {
	static double starttime=0;
	static double curtime=0;
	static double lasttime=0;
	struct timeval tv;

	lasttime=curtime;
	gettimeofday(&tv,NULL);
	curtime = tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0 + 0.5;
	
	if (starttime==0) {
		starttime=curtime;
		lasttime=curtime;
	}
	
	printf("> %s (delta: %.2f ms, total %.2f ms)\n",milestone,curtime-lasttime,curtime-starttime);
}

/** 
 * \brief RIFF Wave file header structure
 */
struct wavhdr_t {
	uint32_t rifftag;			///< Must be "RIFF"
	uint32_t chunksize;			///< Size of data chunk (filesize - 8)
	uint32_t wavetag;			///< Must be "WAVE"
	uint32_t fmttag;			///< Must be "fmt "
	uint32_t fmtsize;			///< Size of format subchunk (16 for PCM)
	uint16_t audioformat;		///< Audio format (1 for PCM)
	uint16_t numchannels;		///< Number of audio channels
	uint32_t samplerate;		///< Sampling rate
	uint32_t byterate;			///< Number of bytes per second (samplerate*numchannels*bitspersample/8)
	uint16_t blockalign;		///< Number of bytes per sample (numchannels*bitspersample/8)
	uint16_t bitspersample;		///< Bits per sample
	uint32_t datatag;			///< Must be "data"
	uint32_t datasize;			///< Size of sample data subchunk
};

/**
 * \brief Voice structure containing rendering information
 */
struct voice_t {
	int active;					///< Nonzero if the sample is currently playing
	int pos;					///< The current sample position
	int start;					///< Time at which the sample should start
	long length;				///< Length of the sample
	float *data;				///< The sample data
};

/**
 * \brief Get a value from a given key in a Ruby hash and do type checking.
 *
 * Raises a Ruby exception if either the key is not found or the value has the wrong type.
 *
 * @param hash The hash to get the value from
 * @param key The key that should be looked up. Must be a Symbol.
 * @param type The Ruby type constant that the value must conform to.
 * @returns The value at hash[key].
 */
static VALUE gethashitem(VALUE hash, VALUE key, int type) {
	VALUE res = rb_hash_aref(hash,key);

	if (NIL_P(res)) {
		rb_raise(rb_eRuntimeError,"Missing key '%s' in one of the samples",rb_id2name(SYM2ID(key)));
	}
	
	if (TYPE(res)!=type) {
		rb_raise(rb_eRuntimeError,"Samples key '%s' is of wrong type",rb_id2name(SYM2ID(key)));
	}

	return res;
}

/**
 * \brief Read a Wave file into a voice_t structure.
 *
 * This function reads the Wave file at \a filename. It checks if the file has 
 * a usable sample format and then populates \a voice by converting the data
 * to floats. 
 *
 * It raises a Ruby exception in case of an error.
 *
 * @param voice A voice_t structure to be populated.
 * @param filename The filename to be read.
 */
static void readsample(struct voice_t *voice, const char *filename) {
	FILE *f;
	if ((f=fopen(filename,"r"))==NULL) {
		rb_raise(rb_eRuntimeError,"Could not open '%s' for reading",filename);
	}
	
	// get filesize
	fseek(f,0,SEEK_END);
	voice->length=ftell(f); // TODO: not length in samples
	rewind(f);

	// read data to a temporary buffer
	uint8_t *data=NULL;
	if (posix_memalign((void**)&data,16,voice->length*sizeof(uint8_t))) {
		fclose(f);
		rb_raise(rb_eRuntimeError,"Error allocating aligned short memory for '%s'",filename);
	}
	if ((fread(data,1,voice->length,f))!=voice->length) {
		fclose(f);
		rb_raise(rb_eRuntimeError,"Error while reading from '%s'",filename);
	}
	fclose(f);

	// some sanity checking of the RIFF header
	struct wavhdr_t *hdr = (struct wavhdr_t *)data;
	// FIXME: currently assuming little-endianness
	if (hdr->rifftag!=0x46464952 || hdr->wavetag!=0x45564157 || hdr->fmttag!=0x20746d66 || hdr->datatag!=0x61746164){
		free(data);
		rb_raise(rb_eRuntimeError,"Bad RIFF header in '%s'.",filename);
	}
	if (hdr->audioformat!=0x01 || hdr->numchannels!=0x02 || hdr->samplerate!=44100 || hdr->byterate!=hdr->samplerate*hdr->numchannels*(hdr->bitspersample>>3) || hdr->bitspersample!=16 || hdr->blockalign!=(hdr->bitspersample>>3)*hdr->numchannels) {
		free(data);
		rb_raise(rb_eRuntimeError,"Bad audio format in '%s'. Only uncompressed 44100Hz 16-bit stereo is supported.",filename);
	}

	// convert samples to float
	voice->length=hdr->datasize>>1;
	if (posix_memalign((void**)&voice->data,16,voice->length*sizeof(float))) {
		rb_raise(rb_eRuntimeError,"Error allocating aligned float memory for '%s'",filename);
	}
	int16_t *shortptr=(int16_t*)(data+sizeof(struct wavhdr_t));
	float *floatptr=voice->data;
	for (int i=0;i<voice->length;++i) {
		*(floatptr++)=((float)*(shortptr++))*(1.0f/32767.0f);
	}

	free(data);
}

/**
 * \brief Render a series of samples to an output file.
 *
 * The samples are given as a list of hashes. Each hash contains a single
 * sample that should be rendered at a specified time value. These hashes
 * must have the following keys:
 *
 * - :filename - The filename of the sample. Must be a 44100 Hz 16 bit 
 *               stereo Wave file. Value must be a String.
 * - :start - The time at which the sample should be started. Specified
 *            as sample number, so 44100 is exactly one second. Value
 *            must be of Numeric type.
 *
 * @param klass The Ruby class/module receiving the call.
 * @param arg_filename The filename that should be created. Must be String.
 * @param arg_samples A list of hashes containing sample information. 
 * @returns Always returns a nil value.
 */
static VALUE requencer_render(VALUE klass, VALUE arg_filename, VALUE arg_samples) {
	Check_Type(arg_filename,T_STRING);
	Check_Type(arg_samples,T_ARRAY);

	char *filename = RSTRING(arg_filename)->ptr;
	
	struct RArray *samples = RARRAY(arg_samples);
	int numsamples = samples->len;
	struct voice_t *voices = ALLOC_N(struct voice_t,numsamples); 

	unsigned int totalsamples = 0;

	BENCHMARK("start");
	
	// process sample list
	VALUE key_filename = ID2SYM(rb_intern("filename"));
	VALUE key_start = ID2SYM(rb_intern("start"));
	for (int i=0;i<numsamples;++i) {

		if (TYPE(samples->ptr[i])!=T_HASH) {
			rb_raise(rb_eRuntimeError,"Index %d of sample array is not a hash",i);
		}

		VALUE filename = gethashitem(samples->ptr[i],key_filename,T_STRING);
		int start = FIX2INT(gethashitem(samples->ptr[i],key_start,T_FIXNUM));

		voices[i].active = 0;
		voices[i].pos = 0;
		voices[i].start = start & ~0x03;
		rb_warn("reading %s starting at %d",RSTRING(filename)->ptr,voices[i].start);
		readsample(&voices[i],RSTRING(filename)->ptr);
		totalsamples = MAX(totalsamples,voices[i].start+voices[i].length);
	}

	rb_warn("file length: %d floats",totalsamples);
	BENCHMARK("file reading");

	// prepare output buffer
	float *output = ALLOC_N(float, totalsamples);
	MEMZERO(output,float,totalsamples);
	BENCHMARK("allocating output buffer");

	// render each sample
	for (int i=0;i<numsamples;++i) {
		int end=voices[i].start+voices[i].length;
		float *dst=output+voices[i].start;
		float *src=voices[i].data;
		for (int j=voices[i].start;j<end;++j) {
			*(dst++)+=*(src++);
		}
	}
	BENCHMARK("rendering");

	// convert to signed shorts in-place
	float *floatptr=output;
	int16_t *shortptr=(int16_t*)output;
	for (int i=0;i<totalsamples;++i) {
		*(shortptr++)=(int16_t)(*(floatptr++)*32767.0f);
	}
	BENCHMARK("float to short");
	
	// encode to mp3
	// TODO: move to method
	FILE *f;
	if ((f=fopen(filename,"w"))==NULL) {
		rb_raise(rb_eRuntimeError,"Could not open '%s' for writing",filename);
	}
	lame_t lame = lame_init();
	lame_set_in_samplerate(lame,44100);
	lame_set_num_channels(lame,2);
	lame_set_asm_optimizations(lame,AMD_3DNOW,1);
	lame_set_asm_optimizations(lame,SSE,1);
	lame_set_VBR(lame,vbr_default);
	lame_set_quality(lame,7);
	lame_init_params(lame);
	int samplesleft=totalsamples>>1;
	int framesize;
	uint8_t mp3buf[20480];
	float *ptr=output;
	while (samplesleft>0) {
		framesize=lame_encode_buffer_interleaved(lame,(short int*)ptr,MIN(samplesleft,4096),mp3buf,20480);
		ptr+=4096;
		samplesleft-=8192;
		if ((fwrite(mp3buf,1,framesize,f))!=framesize) {
			fclose(f);
			lame_close(lame);
			rb_raise(rb_eRuntimeError,"Error encoding mp3");
		}
	}
	framesize=lame_encode_flush(lame,mp3buf,20480);
	if ((fwrite(mp3buf,1,framesize,f))!=framesize) {
		fclose(f);
		lame_close(lame);
		rb_raise(rb_eRuntimeError,"Error encoding mp3");
	}

	fclose(f);
	lame_close(lame);
	BENCHMARK("encoding mp3");

	// free data
	free(output);
	for (int i=0;i<numsamples;++i) {
		if (voices[i].data) {
			free(voices[i].data);
		}
	}
	free(voices);
	BENCHMARK("freeing data");

	return Qnil;
}

/**
 * \brief Ruby extension initialization function.
 *
 * Called when Ruby initializes the C extension. Registers classes, modules 
 * and containing functions.
 */
void Init_requencer() {
	VALUE module = rb_define_module("Requencer");
	rb_define_singleton_method(module,"render",requencer_render,2);
}
