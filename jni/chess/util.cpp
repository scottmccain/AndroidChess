#include <stdio.h>
#include <zip.h>
#include <unistd.h>
#include <fcntl.h>

#include "logging.h"

zip_file* file;
struct zip* APKArchive;

#ifdef __cplusplus
extern "C" {
#endif

void loadAPK (const char* apkPath) {

	int i, numFiles;
	const char* name;

	LOGI("Loading APK %s", apkPath);

	APKArchive = zip_open(apkPath, 0, NULL);
	if (APKArchive == NULL) {
		LOGE("Error loading APK");
		return;
	}

	//Just for debug, print APK contents
	numFiles = zip_get_num_files(APKArchive);
	for (i=0; i<numFiles; i++) {
		name = zip_get_name(APKArchive, i, 0);
		if (name == NULL) {
			LOGE("Error reading zip file name at index %i : %s", i, zip_strerror(APKArchive));
			return;
		}
		LOGI("File %i : %s\n", i, name);
	}

}

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

