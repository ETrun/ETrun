#include "g_local.h"

/*
 Crash handler for Windows only, on coredump
 should be enabled and used to track any bug.
*/

/**
 * Log (and print) a crash message
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

#if defined WIN32
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
#endif

void EnableStackTrace() {
#if defined WIN32
	win32_initialize_handler();
#endif
}

void DisableStackTrace() {
#if defined WIN32
	win32_deinitialize_handler();
#endif
}
