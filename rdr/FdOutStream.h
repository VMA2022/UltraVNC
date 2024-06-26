/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


//
// FdOutStream streams to a file descriptor.
//

#ifndef __RDR_FDOUTSTREAM_H__
#define __RDR_FDOUTSTREAM_H__

#include "OutStream.h"

namespace rdr {

  class FdOutStream : public OutStream {

  public:

    FdOutStream(int fd, int bufSize=0);
    virtual ~FdOutStream();

    int getFd() { return fd; }

    void flush();
    int length();
    void writeBytes(const void* data, int length);

  private:
    int overrun(int itemSize, int nItems);
    int fd;
    int bufSize;
    int offset;
    U8* start;
  };

}

#endif
