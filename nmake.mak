BINNAME= opentube-server
SRCDIR= $(MAKEDIR)\src
OBJSDIR= $(MAKEDIR)\objs
USELIBC= /MT
DEBUGFLAGS= /GS /Z7 /RTC1
RELEASEFLAGS= /O2
CURFLAGS= $(DEBUGFLAGS)
CPPFLAGS= $(CPPFLAGS) $(CURFLAGS) /Oy- $(USELIBC) /DWIN32_LEAN_AND_MEAN /DWIN32 /I$(SRCDIR)\libs\win32\include /Fo$(OBJSDIR)\\

build :
	$(CPP) $(CPPFLAGS) /c $(SRCDIR)\*.c
	$(CPP) $(CPPFLAGS) /I$(SRCDIR) /c $(SRCDIR)\web\utils\captcha.c $(SRCDIR)\web\utils\sha1.c $(SRCDIR)\web\callbacks\callbacks.c
	$(CPP) $(CPPFLAGS) /EHsc /I$(MAKEDIR)\libs\ctpp2\include /c $(SRCDIR)\templates_ctpp.cpp
	$(CPP) $(USELIBC) $(CURFLAGS) /Fe$(OBJSDIR)\$(BINNAME) $(OBJSDIR)\*.obj /link /LIBPATH:$(MAKEDIR)\libs\win32 /DEFAULTLIB:ws2_32 /DEFAULTLIB:advapi32 /DEFAULTLIB:netapi32 /DEFAULTLIB:mswsock /DEFAULTLIB:libctpp2-st /DEFAULTLIB:zlib /DEFAULTLIB:pthreadVC2 /DEFAULTLIB:libintl /DEFAULTLIB:libiconv
all : build
