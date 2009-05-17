/* ***** BEGIN LICENSE BLOCK *****
 * Version: MIT/X11 License
 * 
 * Copyright (c) 2007 Diego Casorran
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * Contributor(s):
 *   Diego Casorran <dcasorran@gmail.com> (Original Author)
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef CASORRAN_SHORTS_H
#define CASORRAN_SHORTS_H

#define sp1( a, b, c)							\
	_sprintf( (a), (b), (ULONG) (c) )
#define sp2( a, b, c, d)						\
	_sprintf( (a), (b), (ULONG) (c), (ULONG) (d) )
#define sp3( a, b, c, d, e)						\
	_sprintf( (a), (b), (ULONG) (c), (ULONG) (d), (ULONG) (e) )
#define sp4( a, b, c, d, e, f)						\
	_sprintf( (a), (b), (ULONG) (c), (ULONG) (d), (ULONG) (e), (ULONG) (f) )
#define sp5( a, b, c, d, e, f, g )					\
	_sprintf( (a), (b), (ULONG) (c), (ULONG) (d), (ULONG) (e), (ULONG) (f), (ULONG) (g) )
#define sp6( a, b, c, d, e, f, g, h )					\
	_sprintf( (a), (b), (ULONG) (c), (ULONG) (d), (ULONG) (e), (ULONG) (f), (ULONG) (g), (ULONG) (h) )
#define sp7( a, b, c, d, e, f, g, h, i )				\
	_sprintf( (a), (b), (ULONG) (c), (ULONG) (d), (ULONG) (e), (ULONG) (f), (ULONG) (g), (ULONG) (h), (ULONG) (i) )
#define sp8( a, b, c, d, e, f, g, h, i, j )				\
	_sprintf( (a), (b), (ULONG) (c), (ULONG) (d), (ULONG) (e), (ULONG) (f), (ULONG) (g), (ULONG) (h), (ULONG) (i), (ULONG) (j) )
#define sp9( a, b, c, d, e, f, g, h, i, j, k )				\
	_sprintf( (a), (b), (ULONG) (c), (ULONG) (d), (ULONG) (e), (ULONG) (f), (ULONG) (g), (ULONG) (h), (ULONG) (i), (ULONG) (j), (ULONG) (k) )
#define sp10( a, b, c, d, e, f, g, h, i, j, k, l)				\
	_sprintf( (a), (b), (ULONG) (c), (ULONG) (d), (ULONG) (e), (ULONG) (f), (ULONG) (g), (ULONG) (h), (ULONG) (i), (ULONG) (j), (ULONG) (k), (ULONG) (l) )
#define sp11( a, b, c, d, e, f, g, h, i, j, k, l, m)				\
	_sprintf( (a), (b), (ULONG) (c), (ULONG) (d), (ULONG) (e), (ULONG) (f), (ULONG) (g), (ULONG) (h), (ULONG) (i), (ULONG) (j), (ULONG) (k), (ULONG) (l), (ULONG) (m) )


#define snp1( a, b, c, d)						\
	_snprintf( (a), (b), (c), (ULONG) (d) )
#define snp2( a, b, c, d, e)						\
	_snprintf( (a), (b), (c), (ULONG) (d), (ULONG) (e) )
#define snp3( a, b, c, d, e, f)						\
	_snprintf( (a), (b), (c), (ULONG) (d), (ULONG) (e), (ULONG) (f) )
#define snp4( a, b, c, d, e, f, g)					\
	_snprintf( (a), (b), (c), (ULONG) (d), (ULONG) (e), (ULONG) (f), (ULONG) (g) )
#define snp5( a, b, c, d, e, f, g, h)					\
	_snprintf( (a), (b), (c), (ULONG) (d), (ULONG) (e), (ULONG) (f), (ULONG) (g), (ULONG) (h) )

#define sf1( a, b)		_stringf( (a), (ULONG) (b) )
#define sf2( a, b, c)		_stringf( (a), (ULONG) (b), (ULONG) (c) )
#define sf3( a, b, c, d)	_stringf( (a), (ULONG) (b), (ULONG) (c), (ULONG) (d) )
#define sf4( a, b, c, d, e)	_stringf( (a), (ULONG) (b), (ULONG) (c), (ULONG) (d), (ULONG) (e) )
#define sf5( a, b, c, d, e, f)	_stringf( (a), (ULONG) (b), (ULONG) (c), (ULONG) (d), (ULONG) (e), (ULONG) (f) )

#endif /* CASORRAN_SHORTS_H */
