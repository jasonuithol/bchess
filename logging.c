#include <stdarg.h>

int logging = 0;
FILE* logFp;

void openLog() {
	logFp = fopen("bchess.log", "a");
	logging = 1;
}

void closeLog() {
	if (logging != 0) {
		fclose(logFp);
		logging = 0;
	}
}
	
void vlogg(char *format, va_list args) {    

	if (logging != 0) {
			
		// Log to file using vfprintf
		vfprintf(logFp,format,args);
//		fflush(logFp);

	}
}

void logg(char *format, ...) {    

	// I'm told that not calling va_begin/va_end is safe.
	if (logging != 0) {

		// Obtain the variable arguments
		va_list args;
		va_start(args,format);

		// Call the *real* logg function
		vlogg(format, args);

		// Mop up the va_list
		va_end(args);

	}
}

void vprint(char* format, va_list args) {

	// Print to console.
	vprintf(format, args);
	fflush(stdout);

	// Log to disk.
	vlogg(format, args);
	
}

void print(char* format, ...) {

	// Obtain the variable arguments
	va_list args;
	va_start(args,format);

	// call the *real* print function
	vprint(format, args);

	// Mop up the va_list
	va_end(args);
	
}

//
// I couldn't think of a reason to pass in a va_list to error()
// plus also makes calling va_end before exit() really awksy.
//
// Cannot call quit() from here because it lives in bchess.c and I ain't moving it.
//
void error(char* format, ...) {

	// Obtain the variable arguments
	va_list args;
	va_start(args,format);

	// Print to console (this also logs to disk)
	vprint(format, args);

	// Mop up the va_list
	va_end(args);
	
	// close the log file, forces a flush.
	closeLog();
	
	// Die
	exit(1);
}

