/* Global variabels */
extern char etc_save[];
extern char buf[];		//buffer for writting some information
extern char *referrer;

int parse_file(char *file_name, char flag); //flag == 0 ->free parsed file
int copy_file(char *file, FILE *out);
