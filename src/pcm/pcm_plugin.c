/**
 * \file pcm/pcm_plugin.c
 * \ingroup PCM
 * \brief PCM Interface
 * \author Jaroslav Kysela <perex@suse.cz>
 * \author Abramo Bagnara <abramo@alsa-project.org>
 * \date 2000-2001
 */
/*
 *  PCM - Common plugin code
 *  Copyright (c) 2000 by Abramo Bagnara <abramo@alsa-project.org>
 *
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/*!

\page pcm_plugins PCM (digital audio) plugins

PCM plugins extends functionality and features of PCM devices.
The plugins take care about various sample conversions, sample
copying among channels and so on.

\section pcm_plugins_slave Slave definition

The slave plugin can be specified directly with a string or the definition
can be entered inside a compound configuration node. Some restrictions can
be also specified (like static rate or count of channels).

\code
pcm_slave.NAME {
	pcm STR		# PCM name
	# or
	pcm { }		# PCM definition
	format STR	# Format or "unchanged"
	channels INT	# Count of channels or "unchanged" string
	rate INT	# Rate in Hz or "unchanged" string
	period_time INT	# Period time in us or "unchanged" string
	buffer_time INT # Buffer time in us or "unchanged" string
}
\endcode

Example:

\code
pcm_slave.slave_rate44100Hz {
	pcm "hw:0,0"
	rate 44100
}

pcm.rate44100Hz {
	type plug
	slave slave_rate44100Hz
}
\endcode

The equivalent configuration (in one compound):

\code
pcm.rate44100Hz {
	type plug
	slave {
		pcm "hw:0,0"
		rate 44100
	}
}
\endcode

*/
  
#include <sys/shm.h>
#include <limits.h>
#include "pcm_local.h"
#include "pcm_plugin.h"

#ifndef DOC_HIDDEN

static snd_pcm_sframes_t
snd_pcm_plugin_undo_read(snd_pcm_t *pcm ATTRIBUTE_UNUSED,
			 const snd_pcm_channel_area_t *res_areas ATTRIBUTE_UNUSED,
			 snd_pcm_uframes_t res_offset ATTRIBUTE_UNUSED,
			 snd_pcm_uframes_t res_size ATTRIBUTE_UNUSED,
			 snd_pcm_uframes_t slave_undo_size ATTRIBUTE_UNUSED)
{
	return -EIO;
}

static snd_pcm_sframes_t
snd_pcm_plugin_undo_write(snd_pcm_t *pcm ATTRIBUTE_UNUSED,
			  const snd_pcm_channel_area_t *res_areas ATTRIBUTE_UNUSED,
			  snd_pcm_uframes_t res_offset ATTRIBUTE_UNUSED,
			  snd_pcm_uframes_t res_size ATTRIBUTE_UNUSED,
			  snd_pcm_uframes_t slave_undo_size ATTRIBUTE_UNUSED)
{
	return -EIO;
}

snd_pcm_sframes_t
snd_pcm_plugin_undo_read_generic(snd_pcm_t *pcm ATTRIBUTE_UNUSED,
				 const snd_pcm_channel_area_t *res_areas ATTRIBUTE_UNUSED,
				 snd_pcm_uframes_t res_offset ATTRIBUTE_UNUSED,
				 snd_pcm_uframes_t res_size ATTRIBUTE_UNUSED,
				 snd_pcm_uframes_t slave_undo_size)
{
	return slave_undo_size;
}

snd_pcm_sframes_t
snd_pcm_plugin_undo_write_generic(snd_pcm_t *pcm ATTRIBUTE_UNUSED,
				  const snd_pcm_channel_area_t *res_areas ATTRIBUTE_UNUSED,
				  snd_pcm_uframes_t res_offset ATTRIBUTE_UNUSED,
				  snd_pcm_uframes_t res_size ATTRIBUTE_UNUSED,
				  snd_pcm_uframes_t slave_undo_size)
{
	return slave_undo_size;
}

void snd_pcm_plugin_init(snd_pcm_plugin_t *plugin)
{
	memset(plugin, 0, sizeof(snd_pcm_plugin_t));
	plugin->undo_read = snd_pcm_plugin_undo_read;
	plugin->undo_write = snd_pcm_plugin_undo_write;
}

int snd_pcm_plugin_close(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	int err = 0;
	if (plugin->close_slave)
		err = snd_pcm_close(plugin->slave);
	free(plugin);
	return 0;
}

int snd_pcm_plugin_nonblock(snd_pcm_t *pcm, int nonblock)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_nonblock(plugin->slave, nonblock);
}

int snd_pcm_plugin_async(snd_pcm_t *pcm, int sig, pid_t pid)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_async(plugin->slave, sig, pid);
}

int snd_pcm_plugin_info(snd_pcm_t *pcm, snd_pcm_info_t * info)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_info(plugin->slave, info);
}

int snd_pcm_plugin_hw_free(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_hw_free(plugin->slave);
}

int snd_pcm_plugin_sw_params(snd_pcm_t *pcm, snd_pcm_sw_params_t *params)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_sw_params(plugin->slave, params);
}

int snd_pcm_plugin_channel_info(snd_pcm_t *pcm, snd_pcm_channel_info_t *info)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_channel_info_shm(pcm, info, plugin->shmid);
}

snd_pcm_state_t snd_pcm_plugin_state(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_state(plugin->slave);
}

int snd_pcm_plugin_delay(snd_pcm_t *pcm, snd_pcm_sframes_t *delayp)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	snd_pcm_sframes_t sd;
	int err = snd_pcm_delay(plugin->slave, &sd);
	if (err < 0)
		return err;
	if (plugin->client_frames)
		sd = plugin->client_frames(pcm, sd);
	*delayp = sd + snd_pcm_mmap_delay(pcm);
	return 0;
}

int snd_pcm_plugin_prepare(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	int err;
	snd_atomic_write_begin(&plugin->watom);
	err = snd_pcm_prepare(plugin->slave);
	if (err < 0) {
		snd_atomic_write_end(&plugin->watom);
		return err;
	}
	plugin->hw_ptr = 0;
	plugin->appl_ptr = 0;
	snd_atomic_write_end(&plugin->watom);
	if (plugin->init) {
		err = plugin->init(pcm);
		if (err < 0)
			return err;
	}
	return 0;
}

static int snd_pcm_plugin_reset(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	int err;
	snd_atomic_write_begin(&plugin->watom);
	err = snd_pcm_reset(plugin->slave);
	if (err < 0) {
		snd_atomic_write_end(&plugin->watom);
		return err;
	}
	plugin->hw_ptr = 0;
	plugin->appl_ptr = 0;
	snd_atomic_write_end(&plugin->watom);
	if (plugin->init) {
		err = plugin->init(pcm);
		if (err < 0)
			return err;
	}
	return 0;
}

int snd_pcm_plugin_start(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_start(plugin->slave);
}

int snd_pcm_plugin_drop(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_drop(plugin->slave);
}

int snd_pcm_plugin_drain(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_drain(plugin->slave);
}

int snd_pcm_plugin_pause(snd_pcm_t *pcm, int enable)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_pause(plugin->slave, enable);
}

snd_pcm_sframes_t snd_pcm_plugin_rewind(snd_pcm_t *pcm, snd_pcm_uframes_t frames)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	snd_pcm_sframes_t n = snd_pcm_mmap_hw_avail(pcm);
	assert(n >= 0);
	if (n > 0) {
		if ((snd_pcm_uframes_t)n > frames)
			n = frames;
		frames -= n;
	}
	if (frames > 0) {
		snd_pcm_sframes_t err;
		/* FIXME: rate plugin */
		if (plugin->slave_frames)
			frames = plugin->slave_frames(pcm, (snd_pcm_sframes_t) frames);
		snd_atomic_write_begin(&plugin->watom);
		err = snd_pcm_rewind(plugin->slave, frames);
		if (err < 0) {
			if (n <= 0) {
				snd_atomic_write_end(&plugin->watom);
				return err;
			}
			goto _end;
		}
		if (plugin->client_frames)
			err = plugin->client_frames(pcm, err);
		snd_pcm_mmap_hw_backward(pcm, (snd_pcm_uframes_t) err);
		n += err;
	} else
		snd_atomic_write_begin(&plugin->watom);
 _end:
	snd_pcm_mmap_appl_backward(pcm, (snd_pcm_uframes_t) n);
	snd_atomic_write_end(&plugin->watom);
	return n;
}

int snd_pcm_plugin_resume(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_resume(plugin->slave);
}

static snd_pcm_sframes_t snd_pcm_plugin_write_areas(snd_pcm_t *pcm,
						    const snd_pcm_channel_area_t *areas,
						    snd_pcm_uframes_t offset,
						    snd_pcm_uframes_t size)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	snd_pcm_t *slave = plugin->slave;
	snd_pcm_uframes_t xfer = 0;
	snd_pcm_sframes_t result;

	while (size > 0) {
		snd_pcm_uframes_t frames = size;
		const snd_pcm_channel_area_t *slave_areas;
		snd_pcm_uframes_t slave_offset;
		snd_pcm_uframes_t slave_frames = ULONG_MAX;
		
		snd_pcm_mmap_begin(slave, &slave_areas, &slave_offset, &slave_frames);
		if (slave_frames == 0)
			break;
		frames = plugin->write(pcm, areas, offset, frames,
				       slave_areas, slave_offset, &slave_frames);
		assert(slave_frames <= snd_pcm_mmap_playback_avail(slave));
		snd_atomic_write_begin(&plugin->watom);
		snd_pcm_mmap_appl_forward(pcm, frames);
		result = snd_pcm_mmap_commit(slave, slave_offset, slave_frames);
		if (result > 0 && (snd_pcm_uframes_t)result != slave_frames) {
			snd_pcm_sframes_t res;
			res = plugin->undo_write(pcm, slave_areas, slave_offset + result, slave_frames, slave_frames - result);
			if (res < 0)
				return xfer > 0 ? xfer : res;
			frames -= res;
		}
		snd_atomic_write_end(&plugin->watom);
		if (result <= 0)
			return xfer > 0 ? xfer : result;
		offset += frames;
		xfer += frames;
		size -= frames;
	}
	return xfer;
}

static snd_pcm_sframes_t snd_pcm_plugin_read_areas(snd_pcm_t *pcm,
						   const snd_pcm_channel_area_t *areas,
						   snd_pcm_uframes_t offset,
						   snd_pcm_uframes_t size)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	snd_pcm_t *slave = plugin->slave;
	snd_pcm_uframes_t xfer = 0;
	snd_pcm_sframes_t result;
	
	while (size > 0) {
		snd_pcm_uframes_t frames = size;
		const snd_pcm_channel_area_t *slave_areas;
		snd_pcm_uframes_t slave_offset;
		snd_pcm_uframes_t slave_frames = ULONG_MAX;
		
		snd_pcm_mmap_begin(slave, &slave_areas, &slave_offset, &slave_frames);
		if (slave_frames == 0)
			break;
		frames = plugin->read(pcm, areas, offset, frames,
				      slave_areas, slave_offset, &slave_frames);
		assert(slave_frames <= snd_pcm_mmap_capture_avail(slave));
		snd_atomic_write_begin(&plugin->watom);
		snd_pcm_mmap_appl_forward(pcm, frames);
		result = snd_pcm_mmap_commit(slave, slave_offset, slave_frames);
		if (result > 0 && (snd_pcm_uframes_t)result != slave_frames) {
			snd_pcm_sframes_t res;
			
			res = plugin->undo_read(slave, areas, offset, frames, slave_frames - result);
			if (res < 0)
				return xfer > 0 ? xfer : res;
			frames -= res;
		}
		snd_atomic_write_end(&plugin->watom);
		if (result <= 0)
			return xfer > 0 ? xfer : result;
		offset += frames;
		xfer += frames;
		size -= frames;
	}
	return xfer;
}


snd_pcm_sframes_t snd_pcm_plugin_writei(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size)
{
	snd_pcm_channel_area_t areas[pcm->channels];
	snd_pcm_areas_from_buf(pcm, areas, (void*)buffer);
	return snd_pcm_write_areas(pcm, areas, 0, size, 
				   snd_pcm_plugin_write_areas);
}

snd_pcm_sframes_t snd_pcm_plugin_writen(snd_pcm_t *pcm, void **bufs, snd_pcm_uframes_t size)
{
	snd_pcm_channel_area_t areas[pcm->channels];
	snd_pcm_areas_from_bufs(pcm, areas, bufs);
	return snd_pcm_write_areas(pcm, areas, 0, size,
				   snd_pcm_plugin_write_areas);
}

snd_pcm_sframes_t snd_pcm_plugin_readi(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size)
{
	snd_pcm_channel_area_t areas[pcm->channels];
	snd_pcm_areas_from_buf(pcm, areas, buffer);
	return snd_pcm_read_areas(pcm, areas, 0, size,
				  snd_pcm_plugin_read_areas);
}

snd_pcm_sframes_t snd_pcm_plugin_readn(snd_pcm_t *pcm, void **bufs, snd_pcm_uframes_t size)
{
	snd_pcm_channel_area_t areas[pcm->channels];
	snd_pcm_areas_from_bufs(pcm, areas, bufs);
	return snd_pcm_read_areas(pcm, areas, 0, size,
				  snd_pcm_plugin_read_areas);
}

snd_pcm_sframes_t snd_pcm_plugin_mmap_commit(snd_pcm_t *pcm,
					     snd_pcm_uframes_t offset ATTRIBUTE_UNUSED,
					     snd_pcm_uframes_t size)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	snd_pcm_t *slave = plugin->slave;
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t appl_offset;
	snd_pcm_sframes_t slave_size;
	snd_pcm_sframes_t xfer;

	if (pcm->stream == SND_PCM_STREAM_CAPTURE) {
		snd_atomic_write_begin(&plugin->watom);
		snd_pcm_mmap_appl_forward(pcm, size);
		snd_atomic_write_end(&plugin->watom);
		return size;
	}
	slave_size = snd_pcm_avail_update(slave);
	if (slave_size < 0)
		return slave_size;
	areas = snd_pcm_mmap_areas(pcm);
	appl_offset = snd_pcm_mmap_offset(pcm);
	xfer = 0;
	while (size > 0 && slave_size > 0) {
		snd_pcm_uframes_t frames = size;
		snd_pcm_uframes_t cont = pcm->buffer_size - appl_offset;
		const snd_pcm_channel_area_t *slave_areas;
		snd_pcm_uframes_t slave_offset;
		snd_pcm_uframes_t slave_frames = ULONG_MAX;
		snd_pcm_sframes_t result;
		int err;

		err = snd_pcm_mmap_begin(slave, &slave_areas, &slave_offset, &slave_frames);
		if (err < 0)
			return xfer > 0 ? xfer : err;
		if (frames > cont)
			frames = cont;
		frames = plugin->write(pcm, areas, appl_offset, frames,
				       slave_areas, slave_offset, &slave_frames);
		snd_atomic_write_begin(&plugin->watom);
		snd_pcm_mmap_appl_forward(pcm, frames);
		result = snd_pcm_mmap_commit(slave, slave_offset, slave_frames);
		snd_atomic_write_end(&plugin->watom);
		if (result > 0 && (snd_pcm_uframes_t)result != slave_frames) {
			snd_pcm_sframes_t res;
			
			res = plugin->undo_write(pcm, slave_areas, slave_offset + result, slave_frames, slave_frames - result);
			if (res < 0)
				return xfer > 0 ? xfer : res;
			frames -= res;
		}
		if (result <= 0)
			return xfer > 0 ? xfer : result;
		if (frames == cont)
			appl_offset = 0;
		else
			appl_offset += result;
		size -= frames;
		slave_size -= frames;
		xfer += frames;
	}
	assert(size == 0);
	return xfer;
}

snd_pcm_sframes_t snd_pcm_plugin_avail_update(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	snd_pcm_t *slave = plugin->slave;
	snd_pcm_sframes_t slave_size;

	slave_size = snd_pcm_avail_update(slave);
	if (pcm->stream == SND_PCM_STREAM_CAPTURE &&
	    pcm->access != SND_PCM_ACCESS_RW_INTERLEAVED &&
	    pcm->access != SND_PCM_ACCESS_RW_NONINTERLEAVED)
		goto _capture;
	if (plugin->client_frames) {
		plugin->hw_ptr = plugin->client_frames(pcm, *slave->hw_ptr);
		if (slave_size <= 0)
			return slave_size;
		return plugin->client_frames(pcm, slave_size);
	} else {
		plugin->hw_ptr = *slave->hw_ptr;
		return slave_size;
	}
 _capture:
 	{
		const snd_pcm_channel_area_t *areas;
		snd_pcm_uframes_t xfer, hw_offset, size;
		
		xfer = snd_pcm_mmap_capture_avail(pcm);
		size = pcm->buffer_size - xfer;
		areas = snd_pcm_mmap_areas(pcm);
		hw_offset = snd_pcm_mmap_hw_offset(pcm);
		while (size > 0 && slave_size > 0) {
			snd_pcm_uframes_t frames = size;
			snd_pcm_uframes_t cont = pcm->buffer_size - hw_offset;
			const snd_pcm_channel_area_t *slave_areas;
			snd_pcm_uframes_t slave_offset;
			snd_pcm_uframes_t slave_frames = ULONG_MAX;
			snd_pcm_sframes_t result;
			int err;

			err = snd_pcm_mmap_begin(slave, &slave_areas, &slave_offset, &slave_frames);
			if (err < 0)
				return xfer > 0 ? xfer : err;
			if (frames > cont)
				frames = cont;
			frames = plugin->read(pcm, areas, hw_offset, frames,
					      slave_areas, slave_offset, &slave_frames);
			snd_atomic_write_begin(&plugin->watom);
			snd_pcm_mmap_hw_forward(pcm, frames);
			result = snd_pcm_mmap_commit(slave, slave_offset, slave_frames);
			snd_atomic_write_end(&plugin->watom);
			if (result > 0 && (snd_pcm_uframes_t)result != slave_frames) {
				snd_pcm_sframes_t res;
				
				res = plugin->undo_read(slave, areas, hw_offset, frames, slave_frames - result);
				if (res < 0)
					return xfer > 0 ? xfer : res;
				frames -= res;
			}
			if (result <= 0)
				return xfer > 0 ? xfer : result;
			if (frames == cont)
				hw_offset = 0;
			else
				hw_offset += frames;
			size -= frames;
			slave_size -= slave_frames;
			xfer += frames;
		}
		return xfer;
	}
}

int snd_pcm_plugin_mmap(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plug = pcm->private_data;
	size_t size = snd_pcm_frames_to_bytes(pcm, (snd_pcm_sframes_t) pcm->buffer_size);
	int id = shmget(IPC_PRIVATE, size, 0666);
	if (id < 0) {
		SYSERR("shmget failed");
		return -errno;
	}
	plug->shmid = id;
	return 0;
}

int snd_pcm_plugin_munmap(snd_pcm_t *pcm)
{
	snd_pcm_plugin_t *plug = pcm->private_data;
	if (shmctl(plug->shmid, IPC_RMID, 0) < 0) {
		SYSERR("shmctl IPC_RMID failed");
			return -errno;
	}
	return 0;
}

int snd_pcm_plugin_status(snd_pcm_t *pcm, snd_pcm_status_t * status)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	snd_pcm_sframes_t err;
	snd_atomic_read_t ratom;
	snd_atomic_read_init(&ratom, &plugin->watom);
 _again:
	snd_atomic_read_begin(&ratom);
	err = snd_pcm_status(plugin->slave, status);
	if (err < 0) {
		snd_atomic_read_ok(&ratom);
		return err;
	}
	status->appl_ptr = plugin->appl_ptr;
	status->hw_ptr = plugin->hw_ptr;
	status->avail = pcm->buffer_size;
	snd_pcm_plugin_delay(pcm, &status->delay);
	if (!snd_atomic_read_ok(&ratom)) {
		snd_atomic_read_wait(&ratom);
		goto _again;
	}
	if (plugin->client_frames)
		status->avail_max = plugin->client_frames(pcm, (snd_pcm_sframes_t) status->avail_max);
	return 0;
}

int snd_pcm_plugin_hw_refine_slave(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return snd_pcm_hw_refine(plugin->slave, params);
}

int snd_pcm_plugin_hw_params_slave(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
	snd_pcm_plugin_t *plugin = pcm->private_data;
	return _snd_pcm_hw_params(plugin->slave, params);
}

snd_pcm_fast_ops_t snd_pcm_plugin_fast_ops = {
	status: snd_pcm_plugin_status,
	state: snd_pcm_plugin_state,
	delay: snd_pcm_plugin_delay,
	prepare: snd_pcm_plugin_prepare,
	reset: snd_pcm_plugin_reset,
	start: snd_pcm_plugin_start,
	drop: snd_pcm_plugin_drop,
	drain: snd_pcm_plugin_drain,
	pause: snd_pcm_plugin_pause,
	rewind: snd_pcm_plugin_rewind,
	resume: snd_pcm_plugin_resume,
	writei: snd_pcm_plugin_writei,
	writen: snd_pcm_plugin_writen,
	readi: snd_pcm_plugin_readi,
	readn: snd_pcm_plugin_readn,
	avail_update: snd_pcm_plugin_avail_update,
	mmap_commit: snd_pcm_plugin_mmap_commit,
};


#endif
