#include <stdio.h>
#include <string.h>
#include <fnmatch.h>

#define MAX_LENGTH 4048    // the maximum length of the buffer that keeps the reading line 

// put the terminating character at the end of the reading line
static void inline find_the_end_of_string(char *line){

    while (*line != '\n')
        ++line;
    
    *line = '\0';
}

int main(int argc, char *argv[]){

    FILE *fp;
    char line[MAX_LENGTH];

    if ((argc > 4 || argc < 4) || (strcmp("--pattern", argv[1]))) {
        printf("Invalid command.\n");
        return 1;
    }

    fp = fopen(argv[3], "r");
    memset(line, 0, sizeof(line));

    if (fp) {
        while (fgets(line, sizeof(line), fp) != NULL) {     // read each line of the file
            find_the_end_of_string(line);
            if(fnmatch(argv[2],line,0) == 0)    // if the the pattern exists in the reading line then print this line
                printf("%s\n",line);

            memset(line,0,sizeof(line));
        }
    }

    fclose(fp);
    return 0;
}
