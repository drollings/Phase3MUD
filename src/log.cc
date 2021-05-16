#include "descriptors.h"
#include "characters.h"
#include "utils.h"
#include "comm.h"

void mudlogf(SInt8 type, Character *ch, bool file, const char *format, ...) {
	va_list args;
	time_t ct = time(0);
	char *time_s = asctime(localtime(&ct));
	Descriptor *i;
	char *mudlog_buf = (char *)malloc(MAX_STRING_LENGTH);
	char tp;

	*(time_s + strlen(time_s) - 1) = '\0';

	if (file)	fprintf(logfile, "%-19.19s :: ", time_s);

	va_start(args, format);
	if (file)
		vfprintf(logfile, format, args);
//	if (level >= 0)
	vsprintf(mudlog_buf, format, args);
	va_end(args);

	if (file)	fprintf(logfile, "\n");
	fflush(logfile);
	
//	if (level < 0)
//		return;

	START_ITER(iter, i, Descriptor *, descriptor_list) {
		if ((STATE(i) == CON_PLAYING) && !PLR_FLAGGED(i->character, PLR_WRITING) && IS_STAFF(i->character)) {
			tp = ((PRF_FLAGGED(i->character, Preference::Log1) ? 1 : 0) +
					(PRF_FLAGGED(i->character, Preference::Log2) ? 2 : 0));

			if ((!ch || i->character->CanSeeStaff(ch)) && (tp >= type))
				i->character->Sendf("&cG[ %s ]&c0\r\n", mudlog_buf);
		}
	}	
	
	FREE(mudlog_buf);
}


void log(const char *format, ...)
{
	va_list args;
	time_t ct = time(0);
	char *time_s = asctime(localtime(&ct));

	time_s[strlen(time_s) - 1] = '\0';

	fprintf(logfile, "%-19.19s :: ", time_s);

	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);

	fprintf(logfile, "\n");
	fflush(logfile);
}
