/*
		Michael Jones
		jonesmi@onid.oregonstate.edu
		
		CS344-400
		Homework 3 myar.c
	
		Note: This program contains source code from CS344 lectures, 
			The Linux Programming Interface, and Piazza discussion
			boards. I believe each use has been sourced. If I failed
			to individually source each reference they are referenced
			here. 
		
*/

#define _BSD_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <ar.h>
#include <dirent.h>

//Following two defines are needed for file_perm_string()
#define STR_SIZE sizeof("rwxrwxrwx")
#define FP_SPECIAL 0


	//append selected files to an archive
void appenderQ(int argc, char* argv[]);
	
	//if archive file does not exist create one
int archive_file_exists(char *argv[]);
	
//void create_file(char *argv[]);
char *file_perm_string(mode_t perm, int flags);
	
	//remove slash from file name of header
void remove_slash(char *new_name);

	//print out the table of contents of the archive
void table_contents(char* argv[], int tv);

	//deletes one selected file. Could not get to remove
	//	all selected files. 
void deleterD(int argc, char *argv[]);

	//adds all regular files to the archive in a directory
void Appender_A_Dir(int argc, char *argv[]);

	//copies selected files out of an archive. Does not delete.
void xtract(int argc, char **argV);


int main(int argc, char *argv[])
{
    int option;
	
    if((option = getopt(argc, argv, "vtqxdA")) != -1) 
	{
		switch (option) {
            case 't':
			{
                if (argc < 3)
				{
					printf("The proper args for -t are ./myar -t <filename>\n");
					exit(EXIT_FAILURE);
				}else
				{
					table_contents(argv, 0); //0 is flag for option t
					break;
				}
			}
            case 'v':
                if (argc < 3)
				{
					printf("The proper args for -t are ./myar -t <filename>\n");
					exit(EXIT_FAILURE);
				}else
				{
					table_contents(argv, 1); //1 is flag for option v
					break;
				}
            case 'q':
                appenderQ(argc, argv);
                break;
            case 'x':
                xtract(argc, argv);
                break;
            case 'd':
                if (argc < 4)
				{
					printf("No file to delete. Leaving archive unharmed.\n");
					exit(EXIT_FAILURE);
				}
				deleterD(argc, argv);
                break;
            case 'A':
				if (argc > 3)
				{
					printf("Too many args for -A, ignoring extra args\n");
				}
				Appender_A_Dir(argc, argv);
                break;
            default: 
                fprintf(stderr, "You did not enter an option\n");
                exit(EXIT_FAILURE);
        }
    }
    else{
        printf("Please enter the correct format.\n");
		printf("./myar -q <archive name> [files to be archived]\n");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

/*This function will extract the user's desired file or files from the 
	current archive. 
	Parameters:		argc, argv
	Returns: 		void
	Purpose:		To remove files from the archive. 
	
*/
void xtract(int argc, char **argV)
{
	struct ar_hdr header;
	int i = 0, file_size, j;
	int hdr_length = sizeof(struct ar_hdr);
	char headerName[15];
	char *old_time;
	int flag = 1;
	int numFiles;
	numFiles = argc;
	int mode;
	int new_fd;	
	struct stat sb;
	time_t older_time;
	struct utimbuf utb;
	
	//int found[numFiles];
	int fd;
	
	if((fd = open(argV[2], O_RDONLY)) == -1)
	{
		perror("Can't open archive to be extracted from");
			exit(EXIT_FAILURE);
	}
		//jump past archive specifier
	lseek(fd, SARMAG, SEEK_SET);
	
	while ((i < numFiles || numFiles == 3) && 
			(read(fd, (char*) &header, hdr_length)) == hdr_length) 
	{
		j = 3;  //3 is used as we are starting at argv[4]
		flag = 1;

		while (j < numFiles || numFiles == 3) 
		{
			sscanf(header.ar_name, "%s", headerName);
			remove_slash(headerName);	
			
				/* if numFiles = 3 then user is requesting extraction
						of all files. 
				   strcmp of course compares user's file to current header
				   if either is true we will extract
				*/
			if ((numFiles == 3) || (strcmp(argV[j], headerName) == 0)) 
			{
				if (stat(argV[2], &sb) == -1)
				{
					perror("stat");					
				}

				file_size = atoi(header.ar_size);
				
					/*Save time from original header for use later when 
						storing to new file. */
				old_time = header.ar_date;
				char content[file_size];
				read(fd, content, file_size);
				sscanf(header.ar_mode, "%o", &mode);
				
				new_fd = open(headerName, O_RDWR | O_CREAT, 0666 , mode);
				if (fd == -1)   //This code taken from lecture. All uses of same 
				{               //here are sourced here.   
					perror("Can't open input file");
					exit(EXIT_FAILURE);
				}
				write(new_fd, content, atoi(header.ar_size));
				
					/*Thanks to Robert Kety who helped me in Piazza u/s this
						Here we are changing the time in the new extracted 
						file back to the original time of the original file
						*/
				older_time = atoi(old_time);
				utb.actime = older_time; 
				utb.modtime = older_time; 
				if (utime(argV[j], &utb) == -1)
				{
					perror("utime");
				}				
					
				lseek(fd, ((file_size % 2)), SEEK_CUR);
				flag = 0;
				i++;
				break;
			} else
			{
				j++;
			}
		}	
		
		if (flag == 1) 
		{		
			file_size = atoi(header.ar_size);
			file_size += (file_size % 2);
			lseek(fd, file_size, SEEK_CUR);
		}
	}
}


/* This function will append all "regular" files in the current 
    directory. 
	Parameters: 
	Returns:  void
	Purpose:  To create an archive from the entire directory.

	Regular file comments per request of 3.6.9
	http://stackoverflow.com/questions/6858452/what-is-a-regular-file-on-unix
	http://en.wikipedia.org/wiki/Unix_file_types
	http://www.unix.com/unix-for-dummies-questions-and-answers/12252-definition-regular-file.html
	
	Regular files are not special files. They can be distinguished by ls -1
		If they start with a "-" they are a regular file. 
		example of files that are not regular are sockets, pipes, device files.
	
	*/
void Appender_A_Dir(int argc, char *argv[])
{
	struct stat file_stat;
	struct dirent *dp;
	DIR *dir = opendir(".");
	char *file_name;
	int a_fd, new_fd, n;
	char header_buff[59];
	char read_buff[1024];
	
		// The following code found at 
		//	http://stackoverflow.com/questions/20265328/c-readdir-beginning
		//       -with-a-dots-instead-of-files
	a_fd = archive_file_exists(argv);

				//get past archives ARMAG
	lseek(a_fd, SARMAG, SEEK_SET);
	
	while ((dp=readdir(dir)) != NULL) 
	{
        if ( !strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") )
        {
            // do nothing 
        }else
		if ( !strcmp(dp->d_name, "Makefile") || !strcmp(dp->d_name, "makefile") )
        {
            // do nothing (don't archive makefile if exists)
        }else
		if ( !strcmp(dp->d_name, "myar.c") || !strcmp(dp->d_name, "myar") )
        {
            // do nothing (don't archive source file)
        }else
		if ( !strcmp(dp->d_name, "myar.o")|| !strcmp(dp->d_name, "sig_demo.o") )
        {
            // do nothing (don't archive .o file)
        }else
		if ( !strcmp(dp->d_name, argv[2]) || !strcmp(dp->d_name, "sig_demo") 
				|| !strcmp(dp->d_name, "*.a"))
        {
            // do nothing (don't archive .a file)
        }			
		else 
		{
            file_name = dp->d_name;
					//compare file to archive file, don't archive archive
			if (strcmp(file_name, argv[2]) == 0)
			{
				// do nothing (found ar file and not archiveing it)
			}
			else
			{

				if((new_fd = open(file_name, O_RDONLY)) == -1)
				{
					perror("Can't open file to be appended");
					exit(EXIT_FAILURE);
				}
				else
				{	
					//help with this section from piazza link @136
					stat(file_name, &file_stat);
					//write header buffer to archive file
					
					
						//test if a regular file some help for code found here
						//http://stackoverflow.com/questions/4989431/how-to-use-
						//			s-isreg-and-s-isdir-posix-macros
					if ( S_ISREG(file_stat.st_mode) == 0)
					{
						// do nothing (not regular file)
					}else
					{									
						snprintf(header_buff, 59, "%-*s%-*d%-*d%-*d%-*o%-*d", 16, 
							strcat(file_name, "/"), 12, (int)file_stat.st_mtime, 6, 
							file_stat.st_uid, 6, file_stat.st_gid, 8, 
							file_stat.st_mode, 10, (int)file_stat.st_size);
						//write header buffer to archive file
						write(a_fd, header_buff, 58);
							//mark end of header.
						write(a_fd, &ARFMAG, 2);
			
						//write contents of new file.
						while((n = read(new_fd, read_buff, 1024)) != 0)
						{
							if((write(a_fd, read_buff, n) == -1))
							{
								perror("Could not write to the archive file");
								exit(EXIT_FAILURE);
							}					
						}
					}
				}
			}
        }
    }
    closedir(dir);
	close(new_fd);
	close(a_fd);
    return;

}

/*This function deletes the first instance of the selected files 
    found from the archive
	FAIL: Please note it at this point only removes one file if 
		multiple files are requested. 
	Parameters:  takes argc and argv from main
	Returns:  	void
	Purpose:	deletes requested files from archive
*/
void deleterD(int argc, char *argv[])
{
	int input_fd, new_fd, i, bytes_written;
	int flag;
	//struct stat file_stat;
	struct ar_hdr myHdr;
	//struct ar_hdr org_header;
	char read_buff[1024];
	char buf[1];
	//char *header;
	int bytes_read;
	//int total_bytes_read;
	char *file_name;//[16];
	int file_size;
	int offset;
	
	bytes_read = 0;
	bytes_written = 0;

			//open original archive file
	if((input_fd = open(argv[2], O_RDONLY)) == -1)
	{
		perror("Can't open file to be appended");
			exit(EXIT_FAILURE);
	}
	lseek(input_fd, SARMAG, SEEK_SET);
	for (i = 3; i < argc; i++)
	{	
		flag = 0;
				/* Searches for a match in a header name  */
		while (read(input_fd, read_buff, 60) == 60) 
		{
			sscanf(read_buff, "%s %s %s %s %s %s %s", myHdr.ar_name, myHdr.ar_date,
               myHdr.ar_uid, myHdr.ar_gid, myHdr.ar_mode, myHdr.ar_size, myHdr.ar_fmag);
			file_size = strtoul(myHdr.ar_size, NULL, 10);
			
				/* Remove slash in ar_name */
			file_name = myHdr.ar_name;
			remove_slash(file_name);
	
			/* found file, set offset */
			if (strcmp(argv[i], file_name) == 0) 
			{
				flag = 1;
				offset = lseek(input_fd, 0, SEEK_CUR);
				lseek(input_fd, 0, SEEK_SET);
				break;
			}
			/* Adds one to file_size if its an odd number 
				http://en.wikipedia.org/wiki/Ar_%28Unix%29
				The data section is 2 byte aligned. If it 
					would end on an odd offset, a '\n' is used as filler.
			*/	
			if (file_size % 2 != 0) 
			{
				file_size += 1;
			}
			/* Seeks to the next header */
			lseek(input_fd, file_size, SEEK_CUR);
		}
		if (flag == 0) //none found
		{
			exit(EXIT_FAILURE);
		}
			/* Adds one to file_size if its an odd number 
				http://en.wikipedia.org/wiki/Ar_%28Unix%29
				The data section is 2 byte aligned. If it 
					would end on an odd offset, a '\n' is used as filler.
			*/
	
		if (file_size % 2 != 0) 
		{
			file_size += 1;
		}
	
			/* Unlink old archive for creation of new one*/
		unlink(argv[2]);
		new_fd = open(argv[2], O_WRONLY | O_CREAT, 0666);
			/* Reads and writes one byte at a time until it reaches
				file_header offset, when it does, skips over file_size and header */
		bytes_written = 0;
		while (read(input_fd, buf, 1) == 1) 
		{
			bytes_written++;
			if (bytes_written == (offset - 60)) 
			{
				lseek(input_fd, file_size + 60, SEEK_CUR);
			}
			write(new_fd, buf, 1);
		}
	}
	close(input_fd);
	close(new_fd);
}

/*This function appends all selected files into the archive
	Parameters:  takes argc and argv from main
	Returns:  	void
	Purpose:	appends files to archive
*/
void appenderQ(int argc, char* argv[])
{
	int a_fd, new_fd, i, n;
	char new_file_name[16];  //stores desired file name of argv
	struct stat file_stat;
	char header_buff[59];
	char read_buff[1024];

		//check to see if desired archive file exists
		//if it doesn't will create
	a_fd = archive_file_exists(argv);

			//get past archives ARMAG
	lseek(a_fd, SARMAG, SEEK_SET);
	
		//argc contains number of files to be attached and starts at element 3
	for (i = 3; i < argc; i++)
	{	//printf("in for %d\n", i);		//for testing number times around loop
		if((new_fd = open(argv[i], O_RDONLY)) == -1)
		{
			perror("Can't open file to be appended");
			exit(EXIT_FAILURE);
		}
		else
		{	
				//help with this section from piazza link @136
			stat(argv[i], &file_stat);
			strcpy(new_file_name, argv[i]);
			
				//write header buffer to archive file
			snprintf(header_buff, 59, "%-*s%-*d%-*d%-*d%-*o%-*d", 16, strcat(new_file_name, "/"), 12, (int)file_stat.st_mtime, 6, file_stat.st_uid, 6, file_stat.st_gid, 8, file_stat.st_mode, 10, (int)file_stat.st_size);
				//write header buffer to archive file
			write(a_fd, header_buff, 58);
				//mark end of header.
			write(a_fd, &ARFMAG, 2);
		
				//write contents of new file.
			while((n = read(new_fd, read_buff, 1024)) != 0)
			{
				if((write(a_fd, read_buff, n) == -1))
				{
					perror("Could not write to the archive file");
					exit(EXIT_FAILURE);
				}
			}
		}
    }
	close(new_fd);
	close(a_fd);
}	

/*
	This function tests if a file already exists and if it does 
		not it will create same with proper header.
	Parameters:  takes argv from calling func
	Returns:  	fd file descriptor
	Purpose:	create a new fd if needed and append ARMAG
*/
int archive_file_exists(char *argv[])
{
	int fd;
			//Code to check if file exists found here
			/* stackoverflow.com/questions/230062/whats-the-best-way-to-check
										-if-a-file-exists-in-c-cross-platform*/
	if( access( argv[2], F_OK ) != -1 ) 
	{
		fd = open(argv[2], O_RDWR | O_CREAT, 0666);
		if (fd == -1)   //This code taken from lecture. All uses of same 
		{               //here are sourced here.   
			perror("Can't open input file");
			exit(EXIT_FAILURE);
		}
		return fd;
	
	} else 
	{
			//If file does not exists. Create a new file and setup
			//ARMAG. Return open file to calling function.
	
		fd = open(argv[2], O_RDWR | O_CREAT, 0666);
		if (fd == -1)   //This code taken from lecture. All uses of same 
		{               //here are sourced here.   
			perror("Can't open input file");
			exit(EXIT_FAILURE);
		}
			//creating !<arch>\n into the new archive file.
		write(fd, ARMAG, SARMAG);
		printf("ar: creating %s\n", argv[2]);
		return fd;
	}
}		
/*
	This function prints to the screen the table of contents for the
		archive. It prints both regular and verbose contents 
	Parameters:  takes argv and flag tv which allows the function
		to know if verbose or regular TOC
	Returns:  	 void
	Purpose:	 display the contents of the ar file to the user.
*/
void table_contents(char *argv[], int tv)
{
	char *new_name;
	struct ar_hdr myHdr;
	char header[20];
	int fd;

	if((fd = open(argv[2], O_RDONLY)) == -1)
	{
		printf("Could not open file or does not exist, Error: %d\n", errno);
		exit(errno);
	}
	else
	{
		if(lseek(fd, SARMAG, SEEK_SET) == -1)
		{
			printf("could not lseek set past Error: %d\n", errno);
			exit(errno);
		}
		else
		{
			while(read(fd, &myHdr, 60) != 0)
			{
					/*The following time info was originally suggested
						by McGrath in post More on HW3 in piazza
						strftime info used from 
						http://www.cplusplus.com/reference/ctime/strftime/*/
				time_t rawtime;
				rawtime = atoi(myHdr.ar_date);
				struct tm *timeinfo;
				timeinfo = localtime(&rawtime);
				strftime(header, 20, "%b %e %R %Y" ,timeinfo);
				
					/*verbose*/
				if (tv == 1)
				{
						//following two lines remove tailing info file name
					new_name = myHdr.ar_name;
					remove_slash(new_name);
					printf("%s %d/%d %6d %s %.*s\n", 
						file_perm_string(strtol(myHdr.ar_mode, NULL, 8), 1),
						atoi(myHdr.ar_uid),atoi(myHdr.ar_gid),
						atoi(myHdr.ar_size), header, 16, new_name);
				}
				else
				{
						//following two lines remove the tailing '/'
					new_name = myHdr.ar_name;
					remove_slash(new_name);
					printf("%.*s\n", 16, new_name);
				}
				lseek(fd, atoi(myHdr.ar_size), SEEK_CUR);
			}
		}
	}
	if((close(fd) == -1))
	{
		printf("Could not close fd%d:, Error: %d\n",fd, errno);
		exit(errno);
	}
}

	//source of code page 296 of text TLPI Kerrisk
char * file_perm_string(mode_t perm, int flags)
{
  static char str[STR_SIZE];
  snprintf(str, STR_SIZE, "%c%c%c%c%c%c%c%c%c",
	   (perm & S_IRUSR) ? 'r' : '-', (perm & S_IWUSR) ? 'w' : '-',
	   (perm & S_IXUSR) ?
	   (((perm & S_ISUID) && (flags & FP_SPECIAL)) ? 's' : 'x') :
	   (((perm & S_ISUID) && (flags & FP_SPECIAL)) ? 'S' : '-'),
	   (perm & S_IRGRP) ? 'r' : '-', (perm & S_IWGRP) ? 'w' : '-',
	   (perm & S_IXGRP) ?
	   (((perm & S_ISGID) && (flags & FP_SPECIAL)) ? 's' : 'x') :
	   (((perm & S_ISGID) && (flags & FP_SPECIAL)) ? 'S' : '-'),
	   (perm & S_IROTH) ? 'r' : '-', (perm & S_IWOTH) ? 'w' : '-',
	   (perm & S_IXOTH) ?
	   (((perm & S_ISVTX) && (flags & FP_SPECIAL)) ? 't' : 'x') :
	   (((perm & S_ISVTX) && (flags & FP_SPECIAL)) ? 'T' : '-'));
  return str;
}

/*This function will remove the '\' after the file name in the header */
void remove_slash(char *new_name) 
{
	int k = sizeof(new_name);
	new_name[k-1] = '\0';
}





