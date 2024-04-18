/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org

    This file written by Ryan C. Gordon (icculus@icculus.org)
*/
#include "SDL_config.h"

/* Output audio to nowhere... */

#include "SDL_rwops.h"
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_LIBRETROaudio.h"
#include "../../video/libretro/libretro.h"

/* The tag name used by DUMMY audio */
#define LIBRETRO_DRIVER_NAME         "LIBRETRO"

/* Audio driver functions */
static int LIBRETRO_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void LIBRETRO_WaitAudio(_THIS);
static void LIBRETRO_PlayAudio(_THIS);
static Uint8 *LIBRETRO_GetAudioBuf(_THIS);
static void LIBRETRO_CloseAudio(_THIS);

extern SDL_AudioDevice *current_audio;
extern size_t libretro_audio_sample_batch_cb(const int16_t* data, size_t frames);

/* background audio buffer*/
static void push_audio(struct SDL_PrivateAudioData* pdata)
{
	SDL_mutexP(pdata->ringbuffer_lock);

    size_t first_chunk = pdata->mixlen;
    size_t second_chunk = 0;	

    if (pdata->ringbuffer_writepos + pdata->mixlen > pdata->ringbuffer_size)
    {
        first_chunk = pdata->ringbuffer_size - pdata->ringbuffer_writepos;
        second_chunk = pdata->mixlen - first_chunk;
    }

    memcpy(pdata->ringbuffer + pdata->ringbuffer_writepos, pdata->mixbuf, first_chunk);
    memcpy(pdata->ringbuffer, pdata->mixbuf+first_chunk, second_chunk);
    pdata->ringbuffer_writepos = (pdata->ringbuffer_writepos + pdata->mixlen) % pdata->ringbuffer_size;

    pdata->ringbuffer_used += pdata->mixlen;

    // check if read buffer was overwritten 
    if (pdata->ringbuffer_used>pdata->ringbuffer_size)
    {
        // correct overwritten read position so it points to oldest data again
        pdata->ringbuffer_readpos = (pdata->ringbuffer_readpos + pdata->ringbuffer_used - pdata->ringbuffer_size) % pdata->ringbuffer_size;
        pdata->ringbuffer_used = pdata->ringbuffer_size;
    }

	SDL_mutexV(pdata->ringbuffer_lock);
}

static void pop_audio(struct SDL_PrivateAudioData* pdata)
{
	SDL_mutexP(pdata->ringbuffer_lock);

	size_t first_chunk = pdata->ringbuffer_used;
	size_t second_chunk = 0;

	if (pdata->ringbuffer_readpos + first_chunk > pdata->ringbuffer_size)
	{
		first_chunk = pdata->ringbuffer_size - pdata->ringbuffer_readpos;
		second_chunk = pdata->ringbuffer_used - first_chunk;
	}

	first_chunk/=4;
	second_chunk/=4;

	// try to submit the first chunk
	size_t submitted=libretro_audio_sample_batch_cb((int16_t*)(pdata->ringbuffer + pdata->ringbuffer_readpos), first_chunk );
	if (submitted == first_chunk && second_chunk>0)
	{
		submitted+=libretro_audio_sample_batch_cb((int16_t*)pdata->ringbuffer, second_chunk );
	}

	submitted*=4;

	pdata->ringbuffer_readpos = (pdata->ringbuffer_readpos + submitted) % pdata->ringbuffer_size;
	pdata->ringbuffer_used-=submitted;

	SDL_mutexV(pdata->ringbuffer_lock);
}

void libretro_upload_audio()
{
	pop_audio(current_audio->hidden);	
}

/* Audio driver bootstrap functions */
static int LIBRETRO_Available(void)
{
/*
	const char *envr = SDL_getenv("SDL_AUDIODRIVER");
	if (envr && (SDL_strcmp(envr, LIBRETRO_DRIVER_NAME) == 0)) {
		return(1);
	}
	return(0);
*/
	return(1);
}

static void LIBRETRO_DeleteDevice(SDL_AudioDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_AudioDevice *LIBRETRO_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;

	/* Initialize all variables that we clean on shutdown */
	this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
	if ( this ) {
		SDL_memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateAudioData *)
				SDL_malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			SDL_free(this);
		}
		return(0);
	}
	SDL_memset(this->hidden, 0, (sizeof *this->hidden));	

	/* Set the function pointers */
	this->OpenAudio = LIBRETRO_OpenAudio;
	this->WaitAudio = LIBRETRO_WaitAudio;
	this->PlayAudio = LIBRETRO_PlayAudio;
	this->GetAudioBuf = LIBRETRO_GetAudioBuf;
	this->CloseAudio = LIBRETRO_CloseAudio;

	this->free = LIBRETRO_DeleteDevice;

	return this;
}

AudioBootStrap LIBRETRO_AUD_bootstrap = {
	LIBRETRO_DRIVER_NAME, "SDL LIBRETRO audio driver",
	LIBRETRO_Available, LIBRETRO_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void LIBRETRO_WaitAudio(_THIS)
{
	/* Don't block on first calls to simulate initial fragment filling. */
	if (this->hidden->initial_calls)
		this->hidden->initial_calls--;
	else
		SDL_Delay(this->hidden->write_delay);
}

static void LIBRETRO_PlayAudio(_THIS)
{	
	push_audio(this->hidden);
}

static Uint8 *LIBRETRO_GetAudioBuf(_THIS)
{
	return(this->hidden->mixbuf);
}

static void LIBRETRO_CloseAudio(_THIS)
{
	if ( this->hidden->mixbuf != NULL ) {
		SDL_FreeAudioMem(this->hidden->mixbuf);
		this->hidden->mixbuf = NULL;
	}

	if (this->hidden->ringbuffer_lock)
	{
		SDL_DestroyMutex(this->hidden->ringbuffer_lock);
		this->hidden->ringbuffer_lock = NULL;
	}

	if (this->hidden->ringbuffer)
	{
		SDL_free(this->hidden->ringbuffer);
		this->hidden->ringbuffer = NULL;
	}
}

static int LIBRETRO_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	float bytes_per_sec = 0.0f;

	/* Allocate mixing buffer */
	this->hidden->mixlen = spec->size;
	this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
	if ( this->hidden->mixbuf == NULL ) {
		return(-1);
	}
	SDL_memset(this->hidden->mixbuf, spec->silence, spec->size);

	bytes_per_sec = (float) (((spec->format & 0xFF) / 8) *
	                   spec->channels * spec->freq);

	/*
	 * We try to make this request more audio at the correct rate for
	 *  a given audio spec, so timing stays fairly faithful.
	 * Also, we have it not block at all for the first two calls, so
	 *  it seems like we're filling two audio fragments right out of the
	 *  gate, like other SDL drivers tend to do.
	 */
	this->hidden->initial_calls = 2;
	this->hidden->write_delay =
	               (Uint32) ((((float) spec->size) / bytes_per_sec) * 1000.0f);

	this->hidden->ringbuffer_lock = SDL_CreateMutex();
	this->hidden->ringbuffer_size = this->hidden->mixlen*2;
	this->hidden->ringbuffer = SDL_malloc(this->hidden->ringbuffer_size);

	/* We're ready to rock and roll. :-) */
	return(0);
}

