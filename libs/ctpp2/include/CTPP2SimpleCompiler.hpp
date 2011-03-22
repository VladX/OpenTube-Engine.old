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
 *      CTPP2SimpleCompiler.hpp
 *
 * $CTPP$
 */
#ifndef _CTPP2_SIMPLE_COMPILER_HPP__
#define _CTPP2_SIMPLE_COMPILER_HPP__ 1

/**
  @file CTPP2SimpleCompiler.hpp
  @brief Compiler, simple API
*/

#include "CTPP2Types.h"
#include "STLString.hpp"
#include <string>
#include <stdio.h>

namespace CTPP // C++ Template Engine
{

// FWD
class CDT;
class OutputCollector;
class VMLoader;
struct VMMemoryCore;

/**
  @class SimpleCompiler CTPP2SimpleCompiler.hpp <CTPP2SimpleCompiler.hpp>
  @brief Simple CTPP2 template compiler
*/
class CTPP2DECL SimpleCompiler
{
public:
	/**
	  @brief Constructor
	  @param sSourceFile - source file
	*/
	SimpleCompiler(const STLW::string & sSourceFile);

	/**
	  @brief Save compiled data to file
	  @param sCompiledFile - compiled file
	  @return 0 - if success, -1 - if any error occured
	*/
	UINT_32 Save(const STLW::string & sCompiledFile) const;

	/**
	  @brief Get memory core
	  @return ready to run memory core
	*/
	const VMMemoryCore * GetCore() const;

	/**
	  @brief A destructor
	*/
	~SimpleCompiler() throw();

private:
	// FWD
	struct _SimpleCompiler;

	/** Pompl for Simple VM internal data */
	_SimpleCompiler    * pSimpleCompiler;

	// Prevent object copying
	SimpleCompiler(const SimpleCompiler & oRhs);
	SimpleCompiler & operator=(const SimpleCompiler & oRhs);
};

} // namespace CTPP
#endif // _CTPP2_SIMPLE_COMPILER_HPP__
// End.
