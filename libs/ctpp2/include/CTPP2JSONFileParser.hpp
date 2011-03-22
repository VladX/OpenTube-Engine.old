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
 *      CTPP2JSONFileParser.hpp
 *
 * $CTPP$
 */
#ifndef _CTPP2_JSON_FILE_PARSER_H__
#define _CTPP2_JSON_FILE_PARSER_H__ 1

#include "CTPP2JSONParser.hpp"

#include <stdio.h>

/**
  @file CTPP2JSONFileParser.hpp
  @brief JSON Parser, file i/o interface
*/

namespace CTPP // C++ Template Engine
{

/**
  @class CTPP2JSONFileParser CTPP2JSONFileParser.hpp <CTPP2JSONFileParser.hpp>
  @brief JSON parser, file i/o interface
*/
class CTPP2DECL CTPP2JSONFileParser:
  private CTPP2JSONParser
{
public:
	/**
	  @brief Constructor
	*/
	CTPP2JSONFileParser(CDT & oICDT);

	/**
	  @brief Parse JSON data from file
	  @param szFile - file name
	  @return 0 if success, -1 if any error occured
	*/
	INT_32 Parse(CCHAR_P szFile);

	/**
	  @brief Parse JSON data from file
	  @param F - opened file descriptor, ready to read data
	  @param iReadBytes - bytes to read
	  @return 0 if success, -1 if any error occured
	*/
	INT_32 Parse(FILE * F, const UINT_32 & iReadBytes);

	/**
	  @brief A destructor
	*/
	~CTPP2JSONFileParser() throw();
private:

};

} // namespace CTPP
#endif // _CTPP2_JSON_FILE_PARSER_H__
// End.
