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
 *      FnSprintf.cpp
 *
 * $CTPP$
 */

#include "CDT.hpp"
#include "CTPP2Logger.hpp"
#include "CTPP2Sprintf.hpp"
#include "FnSprintf.hpp"

namespace CTPP // C++ Template Engine
{

//
// Constructor
//
FnSprintf::FnSprintf()
{
	;;
}

//
// Handler
//
INT_32 FnSprintf::Handler(CDT            * aArguments,
                          const UINT_32    iArgNum,
                          CDT            & oCDTRetVal,
                          Logger         & oLogger)
{
	if (iArgNum == 0)
	{
		oLogger.Emerg("Usage: SPRINTF(format, data1, data2, ..., dataN); at least 2 arguments need");
		return -1;
	}

	CDT oArgs(CDT::ARRAY_VAL);
	INT_32 iPos = iArgNum - 2;

	while (iPos >= 0)
	{
		oArgs.PushBack(aArguments[iPos]);
		--iPos;
	}

	STLW::string sResult;
	FormatString(aArguments[iArgNum - 1].GetString(), sResult, oArgs);
	oCDTRetVal = sResult;

return 0;
}

//
// Get function name
//
CCHAR_P FnSprintf::GetName() const { return "sprintf"; }

//
// A destructor
//
FnSprintf::~FnSprintf() throw() { ;; }

} // namespace CTPP
// End.
