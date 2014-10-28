#include <stdio.h>
#include "defs.h"
#include <zip.h>
#include <unistd.h>
#include <fcntl.h>

zip_file* file;

#ifdef __cplusplus
extern "C" {
#endif

int extract_file(const char *filename, const char *dest) {

	struct zip_stat sb;
	int fd, len;
	long long sum;
	char buf[100];

	file = zip_fopen(APKArchive, filename, 0);
	if (!file) {
		LOGE("Error opening %s from APK", filename);
		return -1;
	}

	if (zip_stat(APKArchive, filename, 0, &sb) == 0) {
		fd = open(dest, O_RDWR | O_TRUNC | O_CREAT, 0644);
		if (fd < 0) {
			LOGE("failed to open destination file: %s/n", dest);
			zip_fclose(file);
			return -1;
		}

		sum = 0;
		while (sum != sb.size) {
			len = zip_fread(file, buf, 100);
			if (len < 0) {
				LOGE("failed to read data from zip file: %s/n", filename);
				zip_fclose(file);
				close(fd);
				return -1;
			}

			write(fd, buf, len);
			sum += len;
		}

		close(fd);
	}

	zip_fclose(file);

	return 0;
}

#ifdef __cplusplus
}
#endif

