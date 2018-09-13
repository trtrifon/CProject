#include <stdio.h>
#include <string.h>

#define MAX_LENGTH 2048     // the maximum length of the buffer that keeps the reading line

// put the terminating character at the end of the reading line
static void inline find_the_end_of_string(char *line) {

    while((*line >= 'a' && *line <= 'z') || (*line >= 'A' && *line <= 'Z') || (*line == ';')) 
        ++line;
    *line = '\0';
}

int main(int argc, char *argv[]) {

    FILE *fp;
    char line[MAX_LENGTH];

    if ((argc > 4 || argc < 4) || (strcmp("--key", argv[1]))) {
        printf("Invalid command.");
        return 1;
    }

    fp = fopen(argv[3], "r");
    memset(line, 0, sizeof(line));

    if (fp) {
        while (fgets(line, sizeof(line), fp) != NULL) {    //read each line of the .csv file
            find_the_end_of_string(line);
            char *tmp = strdup(line); 
            char *tok = strtok(tmp, ";");    // the columns in a .csv file are seperated by ;
            if (!strncmp(tok,argv[2], sizeof(*argv[2])))    // if the element of the first column is the same with the key then print this line
                printf("%s\n",line);
            memset(line,0,sizeof(line));
        }  
    }

    fclose(fp);

    return 0;

}
