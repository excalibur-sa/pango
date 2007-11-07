/*******************************************************************
 *
 *  Copyright 1996-2000 by
 *  David Turner, Robert Wilhelm, and Werner Lemberg.
 *
 *  Copyright 2006  Behdad Esfahbod
 *
 *  This is part of HarfBuzz, an OpenType Layout engine library.
 *
 *  See the file name COPYING for licensing information.
 *
 ******************************************************************/
#ifndef HARFBUZZ_STREAM_PRIVATE_H
#define HARFBUZZ_STREAM_PRIVATE_H

#include "harfbuzz-impl.h"

HB_BEGIN_HEADER

typedef FT_Stream HB_Stream;

HB_INTERNAL HB_Int
_hb_stream_pos( HB_Stream stream );

HB_INTERNAL HB_Error
_hb_stream_seek( HB_Stream stream,
                 HB_UInt   pos );

HB_INTERNAL HB_Error
_hb_stream_frame_enter( HB_Stream stream,
                        HB_UInt   size );

HB_INTERNAL void
_hb_stream_frame_exit( HB_Stream stream );

HB_INTERNAL HB_Error
_hb_face_goto_table( FT_Face   face,
                     HB_UInt   tag );

/* convenience macros */

#define  SET_ERR(c)   ( (error = (c)) != 0 )

#define  GOTO_Table(tag) SET_ERR( _hb_face_goto_table( face, tag ) )
#define  FILE_Pos()      _hb_stream_pos( stream )
#define  FILE_Seek(pos)  SET_ERR( _hb_stream_seek( stream, pos ) )
#define  ACCESS_Frame(size)  SET_ERR( _hb_stream_frame_enter( stream, size ) )
#define  FORGET_Frame()      _hb_stream_frame_exit( stream )

#define  GET_Byte()      (*stream->cursor++)
#define  GET_Short()     (stream->cursor += 2, (HB_Short)( \
				(*(((HB_Byte*)stream->cursor)-2) << 8) | \
				 *(((HB_Byte*)stream->cursor)-1) \
			 ))
#define  GET_Long()      (stream->cursor += 4, (HB_Int)( \
				(*(((HB_Byte*)stream->cursor)-4) << 24) | \
				(*(((HB_Byte*)stream->cursor)-3) << 16) | \
				(*(((HB_Byte*)stream->cursor)-2) << 8) | \
				 *(((HB_Byte*)stream->cursor)-1) \
			 ))


#define  GET_Char()      ((HB_Char)GET_Byte())
#define  GET_UShort()    ((HB_UShort)GET_Short())
#define  GET_ULong()     ((HB_UInt)GET_Long())
#define  GET_Tag4()      GET_ULong()

HB_END_HEADER

#endif /* HARFBUZZ_STREAM_PRIVATE_H */