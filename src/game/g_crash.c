#include "g_local.h"

/**
 * Log (and print) an crash message
 */
void CrashLog(const char *s, qboolean printIt) {
	char       string[1024] = { 0 };
	const char *aMonths[12] =
	{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	qtime_t ct;

	trap_RealTime(&ct);

	if (printIt) {
		G_Printf("%s", s);
	}

	Com_sprintf(string, sizeof (string), "[%s%02d-%02d %02d:%02d:%02d] %s", aMonths[ct.tm_mon], ct.tm_mday, 1900 + ct.tm_year, ct.tm_hour, ct.tm_min, ct.tm_sec, s);

	if (level.crashLog) {
		trap_FS_Write(string, strlen(string), level.crashLog);
	} else {
		G_Printf("CrashLog: error while logging\n");
	}
}

#if defined __linux__

# include <string.h>
# include <unistd.h>
# include <execinfo.h>
# define __USE_GNU
# include <link.h>
# include <sys/ucontext.h>
# include <signal.h>
# include <features.h>

# if __GLIBC__ == 2 && __GLIBC_MINOR__ == 1
#  define GLIBC_21
# endif

extern char *strsignal(int __sig) __THROW;

//use sigaction instead.
void CrashHandler(int signal, siginfo_t *siginfo, ucontext_t *ctx);
void             (*OldHandler)(int signal);
struct sigaction oldact[NSIG];


int segvloop = 0;

void installcrashhandler() {

	struct sigaction act;

	memset(&act, 0, sizeof (act));
	memset(&oldact, 0, sizeof (oldact));
	act.sa_sigaction = (void *)CrashHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &act, &oldact[SIGSEGV]);
	sigaction(SIGILL, &act, &oldact[SIGILL]);
	sigaction(SIGFPE, &act, &oldact[SIGFPE]);
	sigaction(SIGBUS, &act, &oldact[SIGBUS]);

}

void restorecrashhandler() {
	sigaction(SIGSEGV, &oldact[SIGSEGV], NULL);
}

void linux_siginfo(int signal, siginfo_t *siginfo) {
	CrashLog(va("Signal: %s (%d)\n", strsignal(signal), signal), qtrue);
	CrashLog(va("Siginfo: %p\n", siginfo), qtrue);
	if (siginfo) {
		CrashLog(va("Code: %d\n", siginfo->si_code), qtrue);
		CrashLog(va("Faulting Memory Ref/Instruction: %p\n", siginfo->si_addr), qtrue);
	}
}

/* Nico, commenting because it won't compile*/
void linux_dsoinfo() {
	struct link_map *linkmap = NULL;

	ElfW(Ehdr)      * ehdr = (ElfW(Ehdr) *) 0x08048000;
	ElfW(Phdr)      * phdr;
	ElfW(Dyn)       * dyn;
	struct r_debug *rdebug = NULL;

	phdr = (ElfW(Phdr) *)((char *)ehdr + ehdr->e_phoff);

	while (phdr < (ElfW(Phdr) *)((char *)phdr + (ehdr->e_phnum * sizeof (ElfW(Phdr)))) && phdr++) {
		if (phdr->p_type == PT_DYNAMIC) {
			break;
		}
	}

	for (dyn = (ElfW(Dyn) *)phdr->p_vaddr; dyn->d_tag != DT_NULL; dyn++) {
		if (dyn->d_tag == DT_DEBUG) {
			rdebug = (void *)dyn->d_un.d_ptr;
			break;
		}
	}

	CrashLog("DSO Information:\n", qtrue);

	if (rdebug == NULL) {
		CrashLog("rdebug = NULL\n", qtrue);
		return;
	}

	linkmap = rdebug->r_map;

	//rewind to top item.
	while (linkmap->l_prev) {
		linkmap = linkmap->l_prev;
	}

	while (linkmap) {
		if (linkmap->l_addr) {
			if (strcmp(linkmap->l_name, "") == 0) {
				CrashLog(va("0x%08x\t(unknown)\n", linkmap->l_addr), qtrue);
			} else {
				CrashLog(va("0x%08x\t%s\n", linkmap->l_addr, linkmap->l_name), qtrue);
			}
		}
		linkmap = linkmap->l_next;
	}
}

void linux_backtrace(ucontext_t *ctx) {

	// See <asm/sigcontext.h>

	// ctx.eip contains the actual value of eip
	// when the signal was generated.

	// ctx.cr2 contains the value of the cr2 register
	// when the signal was generated.

	// the cr2 register on i386 contains the address
	// that caused the page fault if there was one.

	int i;

	char   **strings;
	void   *array[1024];
	size_t size = (size_t)backtrace(array, 1024);

	//Set the actual calling address for accurate stack traces.
	//If we don't do this stack traces are less accurate.
# ifdef GLIBC_21
	CrashLog(va("Stack frames: %Zd entries\n", size - 1), qtrue);
#  ifndef __x86_64__
	array[1] = (void *)ctx->uc_mcontext.gregs[EIP];
#  else
	array[1] = (void *)ctx->uc_mcontext.gregs[RIP];
#  endif
# else
	CrashLog(va("Stack frames: %zd entries\n", size - 1), qtrue);
#  ifndef __x86_64__
	array[1] = (void *)ctx->uc_mcontext.gregs[REG_EIP];
#  else
	array[1] = (void *)ctx->uc_mcontext.gregs[REG_RIP];
#  endif
# endif
	CrashLog("Backtrace:\n", qtrue);

	strings = (char **)backtrace_symbols(array, (int)size);

	//Start at one and climb up.
	//The first entry points back to this function.
	for (i = 1; i < (int)size; i++)
		CrashLog(va("(%i) %s\n", i, strings[i]), qtrue);

	free(strings);
}

void CrashHandler(int signal, siginfo_t *siginfo, ucontext_t *ctx) {

	//we are real cautious here.
	restorecrashhandler();

	if (signal == SIGSEGV) {
		segvloop++;
	}

	if (segvloop < 2) {
		CrashLog("-8<------- Crash Information ------->8-\n", qtrue);
		CrashLog("---------------------------------------\n", qtrue);
		CrashLog(va("Version: %s %s Linux\n", GAME_VERSION, MOD_VERSION), qtrue);
		CrashLog(va("Map: %s\n", level.rawmapname), qtrue);
		linux_siginfo(signal, siginfo);
		linux_dsoinfo();
		linux_backtrace(ctx);
		CrashLog("-8<--------------------------------->8-\n\n", qtrue);
		CrashLog("Attempting to clean up.\n", qtrue);
		G_ShutdownGame(0);
		//pass control to the default handler.
		if (signal == SIGSEGV) {
			OldHandler = (void *)oldact[SIGSEGV].sa_sigaction;
			(*OldHandler)(signal);
		} else {
			exit(1);
		}
	} else {
		//end this madness we are looping.
		G_Error("Recursive segfault. Bailing out.");
		OldHandler = (void *)oldact[SIGSEGV].sa_sigaction;
		(*OldHandler)(signal);
	}

	return;

}

#elif defined WIN32
# include <windows.h>
# include <process.h>
# include <imagehlp.h>

HMODULE imagehlp = NULL;

typedef BOOL (WINAPI * PFNSYMINITIALIZE)(HANDLE, LPSTR, BOOL);
typedef BOOL (WINAPI * PFNSYMCLEANUP)(HANDLE);
typedef PGET_MODULE_BASE_ROUTINE PFNSYMGETMODULEBASE;
typedef BOOL (WINAPI * PFNSTACKWALK)(DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID, PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE, PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);
typedef BOOL (WINAPI * PFNSYMGETSYMFROMADDR)(HANDLE, DWORD, LPDWORD, PIMAGEHLP_SYMBOL);
typedef BOOL (WINAPI * PFNSYMENUMERATEMODULES)(HANDLE, PSYM_ENUMMODULES_CALLBACK, PVOID);
typedef PFUNCTION_TABLE_ACCESS_ROUTINE PFNSYMFUNCTIONTABLEACCESS;

PFNSYMINITIALIZE          pfnSymInitialize          = NULL;
PFNSYMCLEANUP             pfnSymCleanup             = NULL;
PFNSYMGETMODULEBASE       pfnSymGetModuleBase       = NULL;
PFNSTACKWALK              pfnStackWalk              = NULL;
PFNSYMGETSYMFROMADDR      pfnSymGetSymFromAddr      = NULL;
PFNSYMENUMERATEMODULES    pfnSymEnumerateModules    = NULL;
PFNSYMFUNCTIONTABLEACCESS pfnSymFunctionTableAccess = NULL;

/*
    Visual C 7 Users, place the PDB file generated with the build into your etpub
    directory otherwise your stack traces will be useless.

    Visual C 6 Users, shouldn't need the PDB file since the DLL will contain COFF symbols.

    Watch this space for mingw.
*/

BOOL CALLBACK EnumModules(LPSTR ModuleName, DWORD BaseOfDll, PVOID UserContext) {
	// Nico, unused variable trick fix
	UserContext = UserContext;

	CrashLog(va("0x%08x\t%s\n", BaseOfDll, ModuleName), qtrue);
	return TRUE;
}

char *ExceptionName(DWORD exceptioncode) {
	switch (exceptioncode) {
	case EXCEPTION_ACCESS_VIOLATION: return "Access violation"; break;
	case EXCEPTION_DATATYPE_MISALIGNMENT: return "Datatype misalignment"; break;
	case EXCEPTION_BREAKPOINT: return "Breakpoint"; break;
	case EXCEPTION_SINGLE_STEP: return "Single step"; break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "Array bounds exceeded"; break;
	case EXCEPTION_FLT_DENORMAL_OPERAND: return "Float denormal operand"; break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "Float divide by zero"; break;
	case EXCEPTION_FLT_INEXACT_RESULT: return "Float inexact result"; break;
	case EXCEPTION_FLT_INVALID_OPERATION: return "Float invalid operation"; break;
	case EXCEPTION_FLT_OVERFLOW: return "Float overflow"; break;
	case EXCEPTION_FLT_STACK_CHECK: return "Float stack check"; break;
	case EXCEPTION_FLT_UNDERFLOW: return "Float underflow"; break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO: return "Integer divide by zero"; break;
	case EXCEPTION_INT_OVERFLOW: return "Integer overflow"; break;
	case EXCEPTION_PRIV_INSTRUCTION: return "Privileged instruction"; break;
	case EXCEPTION_IN_PAGE_ERROR: return "In page error"; break;
	case EXCEPTION_ILLEGAL_INSTRUCTION: return "Illegal instruction"; break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "Noncontinuable exception"; break;
	case EXCEPTION_STACK_OVERFLOW: return "Stack overflow"; break;
	case EXCEPTION_INVALID_DISPOSITION: return "Invalid disposition"; break;
	case EXCEPTION_GUARD_PAGE: return "Guard page"; break;
	case EXCEPTION_INVALID_HANDLE: return "Invalid handle"; break;
	default: break;
	}
	return "Unknown exception";
}

void win32_exceptioninfo(LPEXCEPTION_POINTERS e) {
	CrashLog(va("Exception: %s (0x%08x)\n", ExceptionName(e->ExceptionRecord->ExceptionCode), e->ExceptionRecord->ExceptionCode), qtrue);
	CrashLog(va("Exception Address: 0x%08x\n", e->ExceptionRecord->ExceptionAddress), qtrue);
}

void win32_dllinfo() {
	CrashLog("DLL Information:\n", qtrue);
	pfnSymEnumerateModules(GetCurrentProcess(), (PSYM_ENUMMODULES_CALLBACK)EnumModules, NULL);
}

void win32_backtrace(LPEXCEPTION_POINTERS e) {
	PIMAGEHLP_SYMBOL pSym;
	STACKFRAME       sf;
	HANDLE           process, thread;
	DWORD            dwModBase, Disp;
	BOOL             more              = FALSE;
	int              cnt               = 0;
	char             modname[MAX_PATH] = "";

	pSym = (PIMAGEHLP_SYMBOL)GlobalAlloc(GMEM_FIXED, 16384);

	ZeroMemory(&sf, sizeof (sf));
	sf.AddrPC.Offset    = e->ContextRecord->Eip;
	sf.AddrStack.Offset = e->ContextRecord->Esp;
	sf.AddrFrame.Offset = e->ContextRecord->Ebp;
	sf.AddrPC.Mode      = AddrModeFlat;
	sf.AddrStack.Mode   = AddrModeFlat;
	sf.AddrFrame.Mode   = AddrModeFlat;

	process = GetCurrentProcess();
	thread  = GetCurrentThread();

	CrashLog("Backtrace:\n", qtrue);

	for (;; ) {
		more = pfnStackWalk(
		    IMAGE_FILE_MACHINE_I386,
		    process,
		    thread,
		    &sf,
		    e->ContextRecord,
		    NULL,
		    pfnSymFunctionTableAccess,
		    pfnSymGetModuleBase,
		    NULL
		    );

		if (!more || sf.AddrFrame.Offset == 0) {
			break;
		}

		dwModBase = pfnSymGetModuleBase(process, sf.AddrPC.Offset);

		if (dwModBase) {
			GetModuleFileName((HINSTANCE)dwModBase, modname, MAX_PATH);
		} else {
			wsprintf(modname, "Unknown");
		}

		pSym->SizeOfStruct  = sizeof (IMAGEHLP_SYMBOL);
		pSym->MaxNameLength = MAX_PATH;

		if (pfnSymGetSymFromAddr(process, sf.AddrPC.Offset, &Disp, pSym)) {
			CrashLog(va("(%d) %s(%s+%#0x) [0x%08x]\n", cnt, modname, pSym->Name, Disp, sf.AddrPC.Offset), qtrue);
		} else {
			CrashLog(va("(%d) %s [0x%08x]\n", cnt, modname, sf.AddrPC.Offset), qtrue);
		}

		cnt++;
	}

	GlobalFree(pSym);
}

LONG CALLBACK win32_exception_handler(LPEXCEPTION_POINTERS e) {
	char basepath[MAX_PATH];
	char gamepath[MAX_PATH];

	//search path for symbols...
	trap_Cvar_VariableStringBuffer("fs_basepath", basepath, sizeof (basepath));
	trap_Cvar_VariableStringBuffer("fs_game", gamepath, sizeof (gamepath));
	pfnSymInitialize(GetCurrentProcess(), va("%s\\%s", basepath, gamepath), TRUE);
	CrashLog("-8<------- Crash Information ------->8-\n", qtrue);
	CrashLog("---------------------------------------\n", qtrue);
	CrashLog(va("Version: %s %s Win32\n", GAME_VERSION, MOD_VERSION), qtrue);
	CrashLog(va("Map: %s\n", level.rawmapname), qtrue);
	win32_exceptioninfo(e);
	win32_dllinfo();
	win32_backtrace(e);
	CrashLog("-8<--------------------------------->8-\n\n", qtrue);
	CrashLog("Attempting to clean up.\n", qtrue);
	G_ShutdownGame(0);
	pfnSymCleanup(GetCurrentProcess());
	return 1;
}

void win32_initialize_handler(void) {

	imagehlp = LoadLibrary("IMAGEHLP.DLL");
	if (!imagehlp) {
		CrashLog("imagehlp.dll unavailable\n", qtrue);
		return;
	}

	pfnSymInitialize          = (PFNSYMINITIALIZE) GetProcAddress(imagehlp, "SymInitialize");
	pfnSymCleanup             = (PFNSYMCLEANUP) GetProcAddress(imagehlp, "SymCleanup");
	pfnSymGetModuleBase       = (PFNSYMGETMODULEBASE) GetProcAddress(imagehlp, "SymGetModuleBase");
	pfnStackWalk              = (PFNSTACKWALK) GetProcAddress(imagehlp, "StackWalk");
	pfnSymGetSymFromAddr      = (PFNSYMGETSYMFROMADDR) GetProcAddress(imagehlp, "SymGetSymFromAddr");
	pfnSymEnumerateModules    = (PFNSYMENUMERATEMODULES) GetProcAddress(imagehlp, "SymEnumerateModules");
	pfnSymFunctionTableAccess = (PFNSYMFUNCTIONTABLEACCESS) GetProcAddress(imagehlp, "SymFunctionTableAccess");

	if (
	    !pfnSymInitialize ||
	    !pfnSymCleanup ||
	    !pfnSymGetModuleBase ||
	    !pfnStackWalk ||
	    !pfnSymGetSymFromAddr ||
	    !pfnSymEnumerateModules ||
	    !pfnSymFunctionTableAccess
	    ) {
		FreeLibrary(imagehlp);
		CrashLog("imagehlp.dll missing exports.\n", qtrue);
		return;
	}

	// Install exception handler
	SetUnhandledExceptionFilter(win32_exception_handler);
}

void win32_deinitialize_handler(void) {
	// Deinstall exception handler
	SetUnhandledExceptionFilter(NULL);
	pfnSymInitialize          = NULL;
	pfnSymCleanup             = NULL;
	pfnSymGetModuleBase       = NULL;
	pfnStackWalk              = NULL;
	pfnSymGetSymFromAddr      = NULL;
	pfnSymEnumerateModules    = NULL;
	pfnSymFunctionTableAccess = NULL;
	FreeLibrary(imagehlp);
}

#else //other platforms
//FIXME: Get rich, buy a mac, figure out how to do this on OSX.
#endif

void EnableStackTrace() {
#if defined __linux__
	installcrashhandler();
#elif defined WIN32
	win32_initialize_handler();
#else

#endif
}

void DisableStackTrace() {
#if defined __linux__
	restorecrashhandler();
#elif defined WIN32
	win32_deinitialize_handler();
#else

#endif
}
