/*
@author Jonah Groendal
CS2240 A2
2/9/16

This program is given a directory as an argument and prints
every directory and file from within the given directory.
A recursive, depth-first searching method is used. If
this process does not have the permissions needed to read
a directory, it is skipped with a printed notice. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include "apue.h"

#define MAX_CWD_SIZE 1000  // Maximum size of directory character array

// Recursively searches and prints every subdirectory and file
void printAllSubdirs(char *dir);

/* Checks for correct number of arguments then calls
   recursive traversal method printAllSubdirs() */
int main(int argc, char **argv)
{
	if (argc != 2)
		err_quit("Wrong number of arguments");

	printAllSubdirs(*(argv+1));
		
}

/* Recursively traverses subdirectories of given directory
   'dir' in a depth-first fashion and prints every file
   and directory.
*/
void printAllSubdirs(char *dir)
{
	DIR *dp;						// Directory pointer
	struct dirent *pDir;			// Used to get name of directory
	struct stat dirstat;			// Used to check if a directory
	char tempdir[MAX_CWD_SIZE];		// Temporarily holds directory string

	// Open directory if possible
	if ((dp = opendir(dir)) == NULL)
		err_ret("Cannot open directory '%s'", dir);
	else
	{
		/* Recursively loop through each directory and print
		   directory name */
		while ((pDir = readdir(dp)) != NULL)
		{
			// Skip directories '.' and '..'
			if (strcmp(pDir->d_name, "." ) != 0 &&
				strcmp(pDir->d_name, "..") != 0 )
			{
				// Add directory name to full directory string
				strcpy(tempdir, dir);
				strcat(tempdir, "/");
				strcat(tempdir, pDir->d_name);

				lstat(tempdir, &dirstat);		// Get file info
				if (S_ISDIR(dirstat.st_mode))	// Check if directory
				{
				printAllSubdirs(tempdir);		// Recursively call method
				}
				printf(">%s\n", tempdir);		// Print directory name
			}
		}
	}
	closedir(dp);	// Close directory
}