#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>

#define LEN 256

char directoryName[LEN];
int albumSize = 0;
char** origin_name;
char** scale10;
char** scale25;
char** caption;

void findFiles(char* path);
int wildcardMatch(char *s, char* p);
int scaleImage(int num, int scale);
int displayImage(char* fileName);
int rotateImage(int scale, int idx, char* degree);
void cleanup();

int main(int argc, char* argv[]) {
  //  album = malloc(sizeof(album_item) * LEN);
  origin_name = (char**)malloc(sizeof(char*) * LEN);
  scale10 = (char**)malloc(sizeof(char*) * LEN);
  scale25 = (char**)malloc(sizeof(char*) * LEN);
  caption = (char**)malloc(sizeof(char*) * LEN);
  
  if (argc <= 1) {
    findFiles("*");
  } else {
    for (int i = 1; i < argc; i++) {
      findFiles(argv[i]);
    }
  }

  if (albumSize == 0) {
    printf("Album only accept .jpg, .png, and .jpeg files.\n There is no such files in the provided route.\n");
    cleanup();
    exit(0);
  }

  printf("-------------%d photos are selected---------------\n\n", albumSize);
  // scale all the images
  for (int i = 0; i < albumSize; i++) {
    int pid, pidDisplay, status;
    //http://linux.die.net/man/2/waitpid
    pid = scaleImage(i, 10);
    
    // only need to wait for scale10 to finish
    waitpid(pid, &status, WUNTRACED | WCONTINUED);

    if (!WIFEXITED(status)) {
      printf("WIFEXITED(%d) is FALSE\n", status);
      continue;
    }

    pidDisplay = displayImage(scale10[i]);
    
    printf("--------------------------------------------------\n");
    printf("%d photo\n", i + 1);

    //rotate image
    char degree[20];
    memset(degree, 0, 20);
    
    printf("How many degree do you want to rotate?\n");
    fgets(degree, 20, stdin);

    if (strlen(degree) > 9) {
      printf("The input degree is beyond integer. Set to 0 degree\n");
      strcpy(degree, "90");
    } else if (strlen(degree) == 0 || strlen(degree) == 1 || degree[0] == '\r' || degree[0] == '\0'){
      printf("set degree to 90\n");
      strcpy(degree, "90");
    } 
    int d = atoi(degree);

    if (d >= 360 || d <= -360) {
      printf("Convert the degree  between -360 to 360.\n");
      d %= 360;
      memset(degree, 0, 20);
      sprintf(degree, "%d", d);
      printf("degree = %s\n", degree);
    } 
    
    rotateImage(10, i, degree);

    pid = scaleImage(i, 25);
    waitpid(pid, &status, WUNTRACED | WCONTINUED);
    if (WIFEXITED(status)) {
      rotateImage(25, i, degree);
    }

    printf("Please give a nice name to this image:\n");
    char name[LEN];
    memset(name, 0, LEN);
    fgets(name, LEN - 1, stdin);

    caption[i] = (char*)malloc(sizeof(char) * LEN);
    memset(caption[i], 0, LEN);
    
    if (strlen(name) == 0 || strlen(name) == 1 || name[0] == '\0') {
      char *tmp = strrchr(origin_name[i], '/');
      strcpy(caption[i], tmp + 1);
    } else {
      strcpy(caption[i], name);
    }

    // kill the displayed image automatically
    kill(pidDisplay, SIGTERM);
   }
  
  FILE* fp = fopen("album.html", "w+");
  char* header = "<html>\n<h1>a sample index.html</h1>\n<h2>Please click on a thumbnail to view a medium-size image</h2>\n";
  
  if (fp) {
    fputs(header, fp);
  
    for (int i = 0; i < albumSize; i++) {
      char item[LEN];
      memset(item, 0, LEN);
      sprintf(item, "<h2>%s</h2>\n<a href=\"%s\"><img src=\"%s\"></a>\n", caption[i], scale25[i], scale10[i]);
      fputs(item, fp);
    }
    char* theEnd = "</html>";
    fputs(theEnd, fp);
    fclose(fp);
  }
  
  cleanup();
}

void cleanup() {
  for (int i = 0; i < albumSize; i++) {
    free(origin_name[i]);
    free(scale10[i]);
    free(scale25[i]);
    free(caption[i]);
  }
  free(origin_name);
  free(scale10);
  free(scale25);
  free(caption);
}

int rotateImage(int scale, int idx, char* degree) {
  char *fileName;
  int pid;
  
  if (scale == 10) {
    fileName = scale10[idx];
  } else {
    fileName = scale25[idx];
  }

  pid = fork();
  if (pid > 0) {
    return pid;
  } else {
    char route[50];
    memset(route, 0, 50);
    strcpy(route, "/usr/bin/convert");
    execlp(route, "convert", "-rotate", degree, fileName, fileName, NULL);
    printf("Error: rotateImage execl failure.\n");
    _exit(EXIT_FAILURE);
  }
}

int displayImage(char* fileName){
  int pid;
  
  pid = fork();
  if (pid > 0) {
    return pid;
  } else {
    char route[50];
    memset(route, 0, 50);
    strcpy(route, "/usr/bin/display");
    execlp(route, "display", fileName, NULL);
    printf("Error: displayImage execl failure\n");
    _exit(EXIT_FAILURE);
  }
}

int scaleImage(int idx, int scale) {
  char fileName[LEN], scaleInfo[LEN], percent[LEN];
  char *output;
  int pid;
  
  memset(scaleInfo, 0, LEN);
  memset(fileName, 0, LEN);
  memset(percent, 0, LEN);
  
  if (scale == 10) {
    scale10[idx] = (char*)malloc(sizeof(char) * LEN);
    output = scale10[idx];
    strcpy(scaleInfo, "scale10_");
  } else {
    scale25[idx] = (char*)malloc(sizeof(char) * LEN);
    output = scale25[idx];
    strcpy(scaleInfo, "scale25_");
  }
  memset(output, 0, LEN);

  strcpy(fileName, origin_name[idx]);
  
  char* extend = strrchr(fileName, '/');
  strcat(output, scaleInfo);
  strcat(output, extend + 1);
   
  sprintf(percent, "%d%%", scale);
 
  pid = fork();
  if (pid > 0) {
    return pid;
  } else {
    char route[50];
    memset(route, 0, 50);
    strcpy(route, "/usr/bin/convert");
    execl(route, "convert", "-geometry", percent, fileName, output, NULL);
    printf("Error: scaleImage execl failure.\n");
    _exit (EXIT_FAILURE);
  }
}

void findFiles(char* path) {
  char fileName[LEN];
  char inputDir[LEN];
  memset(fileName, 0, LEN);
  memset(inputDir, 0, LEN);
  memset(directoryName, 0, LEN);

  int len = strlen(path);
  char *name = strrchr(path, '/');

  if (len == 0 || strcmp(path, "*") == 0) {
    getcwd(directoryName, LEN);
    strcpy(fileName, "*");
  } else if (name == NULL) {
    strcat(path, "/");
    strcpy(directoryName, path);
    strcpy(fileName, "*");
  } else if (path[len - 1] == '/') {
    strcpy(directoryName, path);
    strcpy(fileName, "*");
  } else {
    strncpy(directoryName, path, name - path);
    strcat(directoryName, "/");
    strcpy(fileName, name + 1);
  }
 
  /*http://www.gnu.org/software/libc/manual/html_node/Simple-Directory-Lister.html*/
  DIR *dp = opendir(directoryName);
  struct dirent *ep;

  if (dp != NULL) {
    ep = readdir(dp);
    while (ep != NULL) {
      char *fName = ep -> d_name;
      char *tmp = strrchr(fName, '.');
      if (tmp == NULL || tmp[0] == '\0') {
	//do nothing
      } else if (strcmp(tmp, ".jpg") != 0 && strcmp(tmp, ".png") != 0 && strcmp(tmp, ".jpeg") != 0) {
	//not picture. do nothing
      } else {
	if (wildcardMatch(fName, fileName) == 1) {
	  origin_name[albumSize] = (char*)malloc(sizeof(char) * LEN);
	  strcpy(origin_name[albumSize], directoryName);
	  strcat(origin_name[albumSize], fName);
	  albumSize++;
	}
      }
      ep = readdir(dp);
    }
    (void) closedir (dp);
  } else {
    perror("Couldn't open the directory");
  }
}

int wildcardMatch(char *s, char* p) {
  int pIdx = 0, sIdx = 0, match = 0, starIdx = -1;
  while (sIdx < strlen(s)) {
    if (pIdx < strlen(p) && (s[sIdx] == p[pIdx] || p[pIdx] == '?')) {
      pIdx++;
      sIdx++;
    } else if (pIdx < strlen(p) && p[pIdx] == '*') {
      match = sIdx;
      starIdx = pIdx;
      pIdx++;
    } else if (starIdx != -1) {
      sIdx = ++match;
      pIdx = starIdx + 1;
    } else {
      return 0;
    }
  }
  while (pIdx < strlen(p) && p[pIdx] == '*') {
    pIdx++;
  }
  if (pIdx == strlen(p)) {
    return 1;
  } else {
    return 0;
  }
}
