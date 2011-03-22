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
 *      CTPP2FileLogger.hpp
 *
 * $CTPP$
 */
#ifndef _CTPP2_FILE_LOGGER_HPP__
#define _CTPP2_FILE_LOGGER_HPP__ 1

#include "CTPP2Logger.hpp"

#include <stdio.h>

/**
  @file CTPP2FileLogger.hpp
  @brief CTPP2 file logger
*/

namespace CTPP // C++ Template Engine
{

/**
  @class FileLogger CTPP2FileLogger.hpp <CTPP2FileLogger.hpp>
  @brief CTPP2 logger subsystem
*/
class FileLogger:
  public Logger
{
protected:
	/** File handle     */
	FILE      * F;

public:
	/**
	  @brief Constructor
	  @param iIBasePriority - base priority
	*/
	FileLogger(FILE * F, const UINT_32  iIBasePriority = CTPP2_LOG_WARNING);

	/**
	  @brief Write message to log file
	  @param iPriority - priority level
	  @param szString - message to store in file
	  @return 0 - if success, -1 - otherwise
	*/
	INT_32 WriteLog(const UINT_32  iPriority,
	                CCHAR_P        szString,
	                const UINT_32  iStringLen);

	/**
	  @brief A destructor
	*/
	~FileLogger() throw();
};

} // namespace CTPP
#endif // _CTPP2_FILE_LOGGER_HPP__
// End.
