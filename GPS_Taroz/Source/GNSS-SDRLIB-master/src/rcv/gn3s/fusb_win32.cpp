/* -*- c++ -*- */
/*
 * Copyright 2003,2005 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sdr.h"
#include "fusb_win32.h"
#include <assert.h>
#include <stdexcept>

static const int MAX_BLOCK_SIZE = 64*1024; // Windows kernel hard limit
static const int DEFAULT_BLOCK_SIZE = MAX_BLOCK_SIZE;
static const int DEFAULT_BUFFER_SIZE = 16 * (1L << 20);		// 16 MB / endpoint

static const int USB_TIMEOUT = 1000;	// in milliseconds

fusb_devhandle *
fusb_sysconfig::make_devhandle (usb_dev_handle *udh)
{
  return new fusb_devhandle_win32 (udh);
}

fusb_devhandle::fusb_devhandle (usb_dev_handle *udh)
  : d_udh (udh)
{
  // that's it
};

fusb_devhandle::~fusb_devhandle ()
{
  // nop
}

// ------------------------------------------------------------------------
// 			     end point handle
// ------------------------------------------------------------------------

fusb_ephandle::fusb_ephandle (int endpoint, bool input_p,
			      int block_size, int nblocks)
  : d_endpoint (endpoint), d_input_p (input_p),
    d_block_size (block_size), d_nblocks (nblocks), d_started (false)
{
  // that't it
}

fusb_ephandle::~fusb_ephandle ()
{
  // nop
}

fusb_devhandle_win32::fusb_devhandle_win32 (usb_dev_handle *udh)
  : fusb_devhandle (udh)
{
  // that's it
}

fusb_devhandle_win32::~fusb_devhandle_win32 ()
{
  // nop
}

fusb_ephandle *
fusb_devhandle_win32::make_ephandle (int endpoint, bool input_p,
				       int block_size, int nblocks)
{
  return new fusb_ephandle_win32 (this, endpoint, input_p,
				    block_size, nblocks);
}

// ----------------------------------------------------------------

fusb_ephandle_win32::fusb_ephandle_win32 (fusb_devhandle_win32 *dh,
					      int endpoint, bool input_p,
					      int block_size, int nblocks)
  : fusb_ephandle (endpoint, input_p, block_size, nblocks),
    d_devhandle (dh), d_input_leftover(0),d_output_short(0)
{
  if (d_block_size < 0 || d_block_size > MAX_BLOCK_SIZE)
    throw std::out_of_range ("fusb_ephandle_win32: block_size");

  if (d_nblocks < 0)
    throw std::out_of_range ("fusb_ephandle_win32: nblocks");

  if (d_block_size == 0)
    d_block_size = DEFAULT_BLOCK_SIZE;

  if (d_nblocks == 0)
    d_nblocks = max (1, DEFAULT_BUFFER_SIZE / d_block_size);

  d_buffer = new char [d_block_size*d_nblocks];
  d_context = new void * [d_nblocks];

  // allocate contexts

  usb_dev_handle *dev = dh->get_usb_dev_handle ();
  int i;

  if (d_input_p)
    endpoint |= USB_ENDPOINT_IN;

  for (i=0; i<d_nblocks; i++)
    usb_bulk_setup_async(dev, &d_context[i], endpoint);
}

fusb_ephandle_win32::~fusb_ephandle_win32 ()
{
  int i;

  stop ();

  for (i=0; i<d_nblocks; i++)
    usb_free_async(&d_context[i]);

  delete [] d_buffer;
  delete [] d_context;
}

bool
fusb_ephandle_win32::start ()
{
  if (d_started)
    return true;	// already running

  d_started = true;

  d_curr = d_nblocks-1;
  d_outstanding_write = 0;
  d_input_leftover =0;
  d_output_short = 0;

  if (d_input_p){	// fire off all the reads
    int i;

    for (i=0; i<d_nblocks; i++) {
      usb_submit_async(d_context[i], (char * ) d_buffer+i*d_block_size,
		      d_block_size);
    }
  }

  return true;
}

bool
fusb_ephandle_win32::stop ()
{
  if (!d_started)
    return true;

  if (!d_input_p)
    wait_for_completion ();

  d_started = false;
  return true;
}

int
fusb_ephandle_win32::write (const void *buffer, int nbytes)
{
  int retval=0;
  char *buf;

  if (!d_started)	// doesn't matter here, but keeps semantics constant
    return -1;

  if (d_input_p)
    return -1;

  int bytes_to_write = nbytes;
  int a=0;

  if (d_output_short != 0) {

	buf = &d_buffer[d_curr*d_block_size + d_block_size - d_output_short];
	a = min(nbytes, d_output_short);
	memcpy(buf, buffer, a);
	bytes_to_write -= a;
	d_output_short -= a;

    if (d_output_short == 0)
        usb_submit_async(d_context[d_curr],
			 &d_buffer[d_curr*d_block_size], d_block_size);

	if (bytes_to_write == 0)
		return nbytes;

	assert(d_output_short == 0);
  }

  d_curr = (d_curr+1)%d_nblocks;
  buf = &d_buffer[d_curr*d_block_size];

  if (d_outstanding_write != (unsigned) d_nblocks) {
    d_outstanding_write++;
  } else {
    retval = usb_reap_async(d_context[d_curr], USB_TIMEOUT);
    if (retval < 0) {
	  	fprintf(stderr, "%s: usb_reap_async: %s\n",
  			__FUNCTION__, usb_strerror());
        return retval;
      }
  }

  memcpy(buf, (void *) &(((char*)buffer)[a]), bytes_to_write);

  d_output_short = d_block_size - bytes_to_write;
  if (d_output_short == 0)
	  usb_submit_async(d_context[d_curr], buf, d_block_size);

  return retval < 0 ? retval : nbytes;
}

int
fusb_ephandle_win32::read (void *buffer, int nbytes)
{
  int retval=0;
  char *buf;

  if (!d_started)	// doesn't matter here, but keeps semantics constant
    return -1;

  if (!d_input_p)
    return -1;

  int bytes_to_read = nbytes;

  int a=0;
  if (d_input_leftover != 0) {

	buf = &d_buffer[d_curr*d_block_size + d_block_size - d_input_leftover];
	a = min(nbytes, d_input_leftover);
	memcpy(buffer, buf, a);
	bytes_to_read -= a;
	d_input_leftover -= a;

    if (d_input_leftover == 0)
        usb_submit_async(d_context[d_curr],
			 &d_buffer[d_curr*d_block_size], d_block_size);

	if (bytes_to_read == 0)
		return nbytes;

	assert(d_input_leftover == 0);
  }


  d_curr = (d_curr+1)%d_nblocks;
  buf = &d_buffer[d_curr*d_block_size];

  retval = usb_reap_async(d_context[d_curr], USB_TIMEOUT);
  if (retval < 0)
  	fprintf(stderr, "%s: usb_reap_async: %s\n",
  			__FUNCTION__, usb_strerror());

  memcpy((void *) &(((char*)buffer)[a]), buf, bytes_to_read);

  d_input_leftover = d_block_size - bytes_to_read;
  if (d_input_leftover == 0)
	  usb_submit_async(d_context[d_curr], buf, d_block_size);

  return retval < 0 ? retval : nbytes;
}

void
fusb_ephandle_win32::wait_for_completion ()
{
  unsigned i;

  for (i=0; i<d_outstanding_write; i++) {
    int context_num;

    context_num = (d_curr+d_outstanding_write+i+1)%d_nblocks;
    usb_reap_async(d_context[context_num], USB_TIMEOUT);
  }

  d_outstanding_write = 0;
}
