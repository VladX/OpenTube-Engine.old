/*-
 * Copyright (c) 2004 - 2011 CTPP Team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the CTPP Team nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      FnVersion.cpp
 *
 * $CTPP$
 */

#include "CDT.hpp"
#include "CTPP2Logger.hpp"
#include "CTPP2Util.hpp"
#include "FnMBSize.hpp"

namespace CTPP // C++ Template Engine
{
//
// Constructor
//
FnMBSize::FnMBSize()
{
	;;
}

//
// Handler
//
INT_32 FnMBSize::Handler(CDT            * aArguments,
                         const UINT_32    iArgNum,
                         CDT            & oCDTRetVal,
                         Logger         & oLogger)
{
	// At least one argument need
	if (iArgNum != 1)
	{
		oLogger.Emerg("Usage: MB_SIZE(data)");
		return -1;
	}

	if (aArguments[0].GetType() != CDT::STRING_VAL) { oCDTRetVal = aArguments[0].Size(); }
	else
	{
		const STLW::string sData = aArguments[0].GetString();

		CCHAR_P  szStart  = sData.data();
		CCHAR_P  szEnd    = szStart + sData.size();
		INT_32   iPos     = 0;
		UINT_32  iCharPos = 0;
		for(;;)
		{
			INT_32 iCharLen = utf_charlen(szStart + iPos, szEnd);
			if (iCharLen == -3) { break; }

			// Check character length
			if (iCharLen < 0) { iCharLen = 1; }

			++iCharPos;
			iPos += iCharLen;
		}
		oCDTRetVal = iCharPos;
	}

return 0;
}

//
// Get function name
//
CCHAR_P FnMBSize::GetName() const { return "mb_size"; }

//
// A destructor
//
FnMBSize::~FnMBSize() throw() { ;; }

} // namespace CTPP
// End.
