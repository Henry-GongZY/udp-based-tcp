#include<cstdio>
#include<cstdlib>
#pragma  comment(lib,"ws2_32.lib")

class FileReader
{
private:
    FILE *f;
    char path_buffer[_MAX_PATH];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

public:

    FILE * selectFile()
    {
        printf("Please input file name:\n");
        scanf("%s",path_buffer);

        if ((f = fopen(path_buffer,"rb")))
        {
            printf("File opened successfully!\n");
            return f;
        }
        else
        {
            printf("File not found, please input again!\n");
            return selectFile();
        }
    }

    char * getFileName()
    {
        _splitpath(path_buffer, drive, dir, fname, ext);
        return strcat(fname, ext);
    }

    FILE * createFile(char *name)
    {
        remove(name);
        if ((f = fopen(name, "ab")))
        {
            printf("File created!\n");
        }
        else
        {
            printf("Failure\n");
        }
        return f;
    }

    bool createDir(char *dir)
    {
        char head[MAX_PATH] = "md ";
        return system(strcat(head, dir));
    }

};
