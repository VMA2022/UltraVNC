/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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


// AVIGenerator.h: interface for the CAVIGenerator class.
//
// A class to easily create AVI
//
// Original code : Example code in WriteAvi.c of MSDN
// 
// Author : Jonathan de Halleux. dehalleux@auto.ucl.ac.be
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AVIGENERATOR_H__6BAF2E9D_3866_4779_A43B_D1B21E7E4F39__INCLUDED_)
#define AFX_AVIGENERATOR_H__6BAF2E9D_3866_4779_A43B_D1B21E7E4F39__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// needed headers
#include <comdef.h>
#include <memory.h>
#include <tchar.h>
#include <string.h>
#include <vfw.h>

#pragma message("     _Adding library: vfw32.lib" ) 
#pragma comment ( lib, "vfw32.lib")

// undefine this if you don't use MFC
//#define _AVIGENERATOR_USE_MFC

/*! \brief A simple class to create AVI video stream.


\par Usage

  Step 1 : Declare an CAVIGenerator object
  Step 2 : Set Bitmap by calling SetBitmapHeader functions + other parameters
  Step 3 : Initialize engine by calling InitEngine
  Step 4 : Send each frames to engine with function AddFrame
  Step 5 : Close engine by calling ReleaseEngine

\par Demo Code:

\code
	CAVIGenerator AviGen;
	BYTE* bmBits;

	// set characteristics
	AviGen.SetRate(20);							// set 20fps
	AviGen.SetBitmapHeader(GetActiveView());	// give info about bitmap

	AviGen.InitEngine();

	..... // Draw code, bmBits is the buffer containing the frame
	AviGen.AddFrame(bmBits);
	.....

	AviGen.ReleaseEngine();
\endcode

\par Update history:

	- {\bf 22-10-2002} Minor changes in constructors.

\author : Jonathan de Halleux, dehalleux@auto.ucl.ac.be (2001)
*/
class CAVIGenerator  
{
public:
	//! \name Constructors and destructors
	//@{
	//! Default constructor 
	CAVIGenerator();
#ifdef _AVIGENERATOR_USE_MFC
	//! Inplace constructor with CView
	CAVIGenerator(LPCTSTR sFileName, CView* pView, DWORD dwRate);
#endif
	//! Inplace constructor with BITMAPINFOHEADER
	CAVIGenerator(LPCTSTR sFileName, LPCTSTR sPath,LPBITMAPINFOHEADER lpbih, DWORD dwRate);
	~CAVIGenerator();
	//@}

	//! \name  AVI engine function
	//@{
	/*! \brief  Initialize engine and choose codex

	 Some asserts are made to check that bitmap has been properly initialized
	*/
	HRESULT InitEngine();

	/*! \brief Adds a frame to the movie. 
	
	The data pointed by bmBits has to be compatible with the bitmap description of the movie.
	*/
	HRESULT AddFrame(BYTE* bmBits);
	//! Release ressources allocated for movie and close file.
	void ReleaseEngine();
	//@}

	//! \name Setters and getters
	//@{
	//! Sets bitmap info to match pView dimension.
//	void SetBitmapHeader(CView* pView);
	//! Sets bitmap info as in lpbih
	void SetBitmapHeader(LPBITMAPINFOHEADER lpbih);
	//! returns a pointer to bitmap info
	LPBITMAPINFOHEADER GetBitmapHeader()							{	return &m_bih;};
	//! sets the name of the ouput file (should be .avi)
	void SetFileName(LPCTSTR _sFileName)							{	strcpy_s(m_sFile,_sFileName); MakeExtAvi();};
	//! Sets FrameRate (should equal or greater than one)
	void SetRate(DWORD dwRate)										{	m_dwRate=dwRate;};
	//@}
	
	//! \name Error handling
	//@{
	//! returns last  error message
	LPCTSTR GetLastErrorMessage() const								{	return m_sError;};
	//@}
	bool read(AVICOMPRESSOPTIONS opts);
	bool save(AVICOMPRESSOPTIONS opts);

protected:	
	//! name of output file
	char m_sFile[1024];
	char mypath[1024];
	//! Frame rate 
	DWORD m_dwRate;	
	//! structure contains information for a single stream
	BITMAPINFOHEADER m_bih;	
	//! last error string
	char m_sError[1024];
	unsigned char *tempbuffer;

private:
	void MakeExtAvi();
	//! frame counter
	long m_lFrame;
	//! file interface pointer
	PAVIFILE m_pAVIFile;
	//! Address of the stream interface
	PAVISTREAM m_pStream;		
	//! Address of the compressed video stream
	PAVISTREAM m_pStreamCompressed; 
};

#endif // !defined(AFX_AVIGENERATOR_H__6BAF2E9D_3866_4779_A43B_D1B21E7E4F39__INCLUDED_)
