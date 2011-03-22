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
 *      CTPP2VMSyscall.cpp
 *
 * $CTPP$
 */
#include "CTPP2VMSyscall.hpp"

namespace CTPP // C++ Template Engine
{

//
// Pre-execution handler setup
//
INT_32 SyscallHandler::PreExecuteSetup(OutputCollector          & oCollector,
                                       CDT                      & oCDT,
                                       const ReducedStaticText  & oSyscalls,
                                       const ReducedStaticData  & oStaticData,
                                       const ReducedStaticText  & oStaticText,
                                       Logger                   & oLogger) { return 0; }

//
// Global Handler initialization
//
INT_32 SyscallHandler::InitHandler(CDT & oCDT) { return 0; }

//
// Get API version
//
INT_32 SyscallHandler::GetVersion() const { return 0; }

//
// Handler resources destructor
//
INT_32 SyscallHandler::DestroyHandler(CDT & oCDT) throw() { return 0; }

//
// A destructor
//
SyscallHandler::~SyscallHandler() throw() { ;; }

} // namespace CTPP
// End.
