#include <nds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <slim.h>

//---------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	//---------------------------------------------------------------------------------

	// Initialise the console, required for printf
	consoleDemoInit();

	if (fatMountSimple("sd:/", get_io_dsisd()))
	{
		if (chdir("sd:/")) {
			iprintf("chdir failed\n");
		}
		if (chdir("homebrew")) {
			iprintf("chdir failed\n");
		}
		
		if (chdir("..")) {
			iprintf("chdir failed\n");
		}
		DIR *pdir;
		struct dirent *pent;

		pdir = opendir("sd:/");

		if (pdir)
		{

			while ((pent = readdir(pdir)) != NULL)
			{
				if (strcmp(".", pent->d_name) == 0 || strcmp("..", pent->d_name) == 0)
					continue;
				if (pent->d_type == DT_DIR)
					iprintf("[%s]\n", pent->d_name);
				else
					iprintf("%s\n", pent->d_name);
			}
			closedir(pdir);
		}
		else
		{
			iprintf("opendir() failure; terminating\n");
		}

		iprintf("testing write...\n");
		FILE* ftest = fopen("sd:/test_a.txt", "w");
		if (ftest) {
			iprintf("open ok!\n");
			fputs("Hello World from FatFs!", ftest);
			iprintf("puts ok!\n");
			fclose(ftest);
			iprintf("test ok!\n");
		} else {
			iprintf("open failed!");
		}

		iprintf("testing read...\n");
		FILE* ftest_r = fopen("sd:/test_a.txt", "rb");
		char testbuf[256] = {0};
		if (ftest_r) {
			int read = fread(testbuf, 1, 256, ftest_r);
			printf("read %d bytes\n", read);
			printf("result: %s\n", testbuf);
			fclose(ftest_r);
			iprintf("test ok!\n");
		} else {
			iprintf("open failed!");
		}

	}
	else
	{
		iprintf("fatInitDefault failure: terminating\n");
	}

	while (1)
	{
		swiWaitForVBlank();
		scanKeys();
		if (keysDown() & KEY_START)
			break;
	}

	return 0;
}
