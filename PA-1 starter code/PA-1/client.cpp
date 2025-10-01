/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Sam Scarcelli
	UIN: 933008350
	Date: 9/22/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	bool newChannel = false;

	/*4.1 Run the server as a child process */
	pid_t pid = fork();
	if (pid == 0) {
		execl("./server", "server", (char*) NULL);
		EXITONERROR("exec failure");
		exit(1);
	}
	else if (pid < 0) {
		EXITONERROR("fork failure");
		exit(1);
	}
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:n")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'n':
				newChannel = true;
				break;
		}
	}

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
	
	/*4.2 Requesting Data Points*/
    char buf[MAX_MESSAGE]; 
	if (filename.empty() && t != 0.0 && e != 0) {
		datamsg d(p, t, e);
		memcpy(buf, &d, sizeof(datamsg));
		chan.cwrite(buf,sizeof(datamsg));
		double reply;
		chan.cread(&reply, sizeof(double)); 
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		
	}

	else if (filename.empty() && t == 0.0 && e == 1) {
		system("mkdir -p recieved");
    	ofstream ofs("received/x1.csv");
        for (int i = 0; i < 1000; ++i) {
            double time = i * 0.004;

            datamsg d1(p, time, 1);
            datamsg d2(p, time, 2);
            double ecg1, ecg2;

            memcpy(buf, &d1, sizeof(datamsg));
            chan.cwrite(buf, sizeof(datamsg));
            chan.cread(&ecg1, sizeof(double));

            memcpy(buf, &d2, sizeof(datamsg));
            chan.cwrite(buf, sizeof(datamsg));
            chan.cread(&ecg2, sizeof(double));

            ofs << time << "," << ecg1 << "," << ecg2 << endl;
        }
        ofs.close();
        cout << "Saved 1000 data points to x1.csv" << endl;
    }

    /*4.3 Requesting Files*/
	else if (!filename.empty()) {
		filemsg fm(0, 0);
		int requestLength = sizeof(filemsg) + filename.size() + 1;
		char* requestBuffer = new char[requestLength];
		memcpy(requestBuffer, &fm, sizeof(filemsg));
		strcpy(requestBuffer + sizeof(filemsg), filename.c_str());
		chan.cwrite(requestBuffer, requestLength);
		long file_length;
		chan.cread(&file_length, sizeof(long));
		delete[] requestBuffer;
	
		string outpath = "received/" + filename;
		int fd = open(outpath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd < 0) {
			perror("open failed");
			exit(1);
		}

		const int CHUNK_SIZE = MAX_MESSAGE - sizeof(filemsg) - filename.size() - 1;
		long offset = 0;

		while (offset < file_length) {
			long remaining = file_length - offset;
			long chunk = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;

			filemsg fm_chunk(offset, chunk);
			int requestLength = sizeof(filemsg) + filename.size() + 1;
			char* requestBuffer = new char[requestLength];
			memcpy(requestBuffer, &fm_chunk, sizeof(filemsg));
			strcpy(requestBuffer + sizeof(filemsg), filename.c_str());

			chan.cwrite(requestBuffer, requestLength);

			char* dataBuffer = new char[chunk];
			chan.cread(dataBuffer, chunk);
			write(fd, dataBuffer, chunk);

			delete[] requestBuffer;
			delete[] dataBuffer;

			offset += chunk;
		}

		close(fd);
		cout << "File received and saved to " << outpath << endl;
	}

	/*4.4 Requesting a new channel*/
	FIFORequestChannel* activeChannel = &chan;

	if (newChannel) {
		MESSAGE_TYPE newChannelMessage = NEWCHANNEL_MSG;
		chan.cwrite(&newChannelMessage, sizeof(MESSAGE_TYPE));

		char newChannelName[MAX_MESSAGE];
		chan.cread(newChannelName, MAX_MESSAGE);

		activeChannel = new FIFORequestChannel(newChannelName, FIFORequestChannel::CLIENT_SIDE);
		cout << "Switched to new channel: " << newChannelName << endl;
	}
	
	/*4.5 Closing the channel*/  
    MESSAGE_TYPE m = QUIT_MSG;
	activeChannel->cwrite(&m, sizeof(MESSAGE_TYPE));
	if (activeChannel != &chan) {
		chan.cwrite(&m, sizeof(MESSAGE_TYPE));
		delete activeChannel;
	}
}
