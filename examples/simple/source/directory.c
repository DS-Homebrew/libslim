#include <nds.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <slim.h>
#include <nds/debug.h>

//---------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	//---------------------------------------------------------------------------------

	// Initialise the console, required for printf
	consoleDemoInit();

	if (fatInitDefault())
	{
		struct statvfs statv;
		struct stat st;
		nocashMessage("fatInitOk");

		sassert(access("sd:/", F_OK) == 0, "access failed!");
		sassert(access("sd:/roms", F_OK) == 0, "access failed!");
		
		if (0 != argc ) {
			int i;
			for (i=0; i<argc; i++) {
				if (argv[i]) printf("[%d] %s\n", i, argv[i]);
			}
		} else {
			printf("No arguments passed!\n");
		}


		nocashWrite(__system_argv->argv[0], strlen(__system_argv->argv[0]));
		if (statvfs("sd:/", &statv) != 0)
		{
			nocashMessage("statvfs failed");
			iprintf("statvfs failed\n");
		}
		if (chdir("sd:/"))
		{
			iprintf("chdir failed\n");
		}

		mkdir("test_dir", 0777);
		chdir("test_dir");

		if (chdir(".."))
		{
			iprintf("chdir failed\n");
		}
		if (rmdir("sd:/test_dir/") != 0) {
			nocashMessage("rmdir failed");
			iprintf("rmdir failed\n");
		}
		DIR *pdir;
		struct dirent *pent;

		pdir = opendir(".");
		char buffer[1024];
		if (pdir)
		{

			while ((pent = readdir(pdir)) != NULL)
			{
				stat(pent->d_name, &st);
				
				if (pent->d_type == DT_DIR)
				{
					sprintf(buffer, "[%s] (%d): %ld\n", pent->d_name, FAT_getAttr(pent->d_name), st.st_ino);
					printf(buffer);
					nocashWrite(buffer, strlen(buffer) - 1);
				}
				else
				{
					sprintf(buffer, "%s (%d): %ld\n", pent->d_name, FAT_getAttr(pent->d_name), st.st_ino);
					printf(buffer);
					nocashWrite(buffer, strlen(buffer) - 1);
				}
			}
			closedir(pdir);
		}
		else
		{
			iprintf("opendir() failure; terminating\n");
		}

		iprintf("testing write...\n");

		FILE *ftest = fopen("sd:/test_a.txt", "w");
		if (ftest)
		{
			iprintf("open ok!\n");
			fputs("Hello World from FatFs!", ftest);
			iprintf("puts ok!\n");
			fclose(ftest);
			iprintf("test ok!\n");
		}
		else
		{
			iprintf("open failed!");
		}

		FAT_setAttr("test.txt", ATTR_HIDDEN);

		iprintf("testing read...\n");
		FILE *ftest_r = fopen("sd:/test_a.txt", "rb");
		char testbuf[256] = {0};
		if (ftest_r)
		{
			int read = fread(testbuf, 1, 256, ftest_r);
			printf("read %d bytes\n", read);
			printf("result: %s\n", testbuf);
			fclose(ftest_r);
			iprintf("test ok!\n");
		}
		else
		{
			iprintf("open failed!");
		}
		char label[256];
		fatGetVolumeLabel("sd:", label);
		printf("label: %s\n", label);

		char cwd[1024];
		getcwd(cwd, 1024);
		printf("cwd: %s", cwd);

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
