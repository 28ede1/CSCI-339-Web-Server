
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h> 

void read_file(int connfd, const char *path){
	int file_fd = open(path, O_RDONLY);
	if (file_fd == -1){ 
		if (errno == ENOENT) { //shows us why open () failed
			printf("404: file does not exist\n");
	} 	else if (errno == EACCES){
			printf("403: permission denied\n"); 
	} 	else {
			perror("open"); //print another error, if one occured
	}
	return; //stop since there is no open file to actually read into 
}
	char buffer[1048];
	ssize_t bytes_read; 
	// Read a chunk from the file into thwe buffer
	while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
		write(connfd, buffer, bytes_read); 
// saves the number of bytes read in bytes read returns a positive number afterwards 
// for write() sends the bytes to the client 
} 
	if (bytes_read == -1){
		perror("read"); 

}
	close (file_fd);
}
