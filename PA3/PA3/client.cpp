#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>
#include <sys/time.h>
using namespace std;

int buffercapacity = MAX_MESSAGE;

void request_data(FIFORequestChannel channel){

}

int main(int argc, char *argv[]){

	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1; // ecg number
	string filename = "";
	bool newchannel = false; // check if a new channel exist
	bool datareq = false; // check if a data point request exist
	bool filereq = false; // check if a file request exist
	bool checktime = false; // check if the user just want first 1K data points
	bool manual_buff = false;
	// take all the arguments first because some of these may go to the server
	while ((opt = getopt(argc, argv, "cp:t:e:f:m:")) != -1) {
		switch (opt) {
			case 'c':
                newchannel = true;
                break;
			case 'p':
                p = atoi (optarg);
				datareq = true;
                break;
            case 't':
                t = atof (optarg);
				checktime = true;
                break;
            case 'e':
                e = atoi (optarg);
                break;
			case 'm':
                buffercapacity = atoi (optarg);
				manual_buff = true;
                break;
			case 'f':
				filename = optarg;
				filereq = true;
				break;
		}
	}

	int pid = fork ();
	if (pid < 0){
		EXITONERROR ("Could not create a child process for running the server");
	}
	if (!pid){ // The server runs in the child process
		if(manual_buff){
			char* args[] = {"./server","-m",(char*) to_string(buffercapacity).c_str(), nullptr};
			if (execvp(args[0], args) < 0){
			EXITONERROR ("Could not launch the server");
			}
		}
		else{
			char* args[] = {"./server", nullptr};
			if (execvp(args[0], args) < 0){
			EXITONERROR ("Could not launch the server");
			}
		}
		
		
	}

	FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

	if( (newchannel&&datareq) || (newchannel&&filereq)){
		REQUEST_TYPE_PREFIX Channelmsg = NEWCHAN_REQ_TYPE;
		chan.cwrite(&Channelmsg , sizeof(NEWCHAN_REQ_TYPE));
		char channelname [30];
		chan.cread(&channelname, sizeof(channelname));
		FIFORequestChannel newchan (channelname, FIFORequestChannel::CLIENT_SIDE);
		cout<<"New Channel Name is: "<<channelname<<endl;
		
		if(datareq){
			DataRequest d (p, t, e);
			newchan.cwrite (&d, sizeof (DataRequest)); // question
			double reply;
			newchan.cread (&reply, sizeof(double)); //answer
			if (isValidResponse(&reply)){
				cout << "\nFor person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl;
			}



			// reset the value for 1000 data points
			p = 1;
			double t1 = 0.0;
			int ecg1 = 1;// when input ecg is 1
			int ecg2 = 2;// when input ecg is 2
			double reply1; // the ecg output to the csv file
			DataRequest d1 (p, t1, ecg1);
			DataRequest d2 (p, t1, ecg2);
			ofstream csv_file;
			string output_x1 = "received/X1.csv";
			csv_file.open(output_x1);

			struct timeval start, end; // https://www.geeksforgeeks.org/measure-execution-time-with-high-precision-in-c-c/
			gettimeofday(&start, NULL); // start the timer
			
			for(int i=0; i<1000; i++){
				newchan.cwrite(&d1, sizeof(d1));
				newchan.cread (&reply1, sizeof(double));
				csv_file << d1.seconds<<"," << reply1 <<","; // output when ecgvalue is 1
				
				newchan.cwrite(&d2, sizeof(d2));
				newchan.cread (&reply1, sizeof(double));
				csv_file <<reply1 <<endl; // output when ecgvalue is 2

				d1.seconds = d1.seconds +0.004; // the time is increment by 4 mili seconds
				d2.seconds = d2.seconds +0.004;
			}
			gettimeofday(&end, NULL);// end the timer

			double total_time;
			total_time = (end.tv_sec - start.tv_sec)+(end.tv_usec - start.tv_usec)* 1e-6 ; // seconds + microseconds
			cout << "Time taken to ouput x1.csv is: " << fixed << total_time <<" sec" << endl;
			cout<<endl;
		}


		else{
			FileRequest fm (0,0);
			int len = sizeof (FileRequest) + filename.size()+1;
			char buf2 [len];
			memcpy (buf2, &fm, sizeof (FileRequest));
			strcpy (buf2 + sizeof (FileRequest), filename.c_str());
			newchan.cwrite (buf2, len);  
			int64 filelen;
			newchan.cread (&filelen, sizeof(int64));
			if (isValidResponse(&filelen)){
				cout << "\nFile length is: " << filelen << " bytes" << endl;
			}


			//////////// editing
			FileRequest *filemessage = (FileRequest*) buf2;
			string output_path =  "received/" + filename;
			FILE* output_file = fopen(output_path.c_str(),"w"); // write to the file
			filemessage->offset = 0;
			char* buf_receive = new char[buffercapacity];
			
			struct timeval start1, end1;// start counting the time
			gettimeofday(&start1, NULL);
			int remain_len = filelen;

			while (remain_len>0){
				filemessage->length =  min( (int) remain_len,  buffercapacity);
				newchan.cwrite(buf2, len);
				newchan.cread(buf_receive,buffercapacity);
				fwrite(buf_receive, 1, filemessage->length, output_file);
				filemessage->offset+=filemessage->length;
				remain_len = remain_len - filemessage->length;
			}
			gettimeofday(&end1, NULL);// end the timer

			double total_time1;
			total_time1 = (end1.tv_sec - start1.tv_sec)+(end1.tv_usec - start1.tv_usec)* 1e-6 ; // seconds + microseconds
			cout << "Time taken transfer file is: " << fixed << total_time1 <<" sec" << endl;
			cout<< "Finished File Transfer!!!"<<endl;
			cout<<endl;
		}
		Request q (QUIT_REQ_TYPE);
    	newchan.cwrite (&q, sizeof (Request));
	}

	else{
		if(datareq){ // ask for data point request
		if(checktime){
			DataRequest d (p, t, e);
			chan.cwrite (&d, sizeof (DataRequest)); // question
			double reply;
			chan.cread (&reply, sizeof(double)); //answer
			if (isValidResponse(&reply)){
				cout << "\nFor person " << p <<", at time " << t << ", the value of ecg "<< e <<" is " << reply << endl;
			}
		}
		



		// reset the value for 1000 data points
		//p = 1;
		else{
			double t1 = 0.0;
			int ecg1 = 1;// when input ecg is 1
			int ecg2 = 2;// when input ecg is 2
			double reply1; // the ecg output to the csv file
			DataRequest d1 (p, t1, ecg1);
			DataRequest d2 (p, t1, ecg2);
			ofstream csv_file;
			string output_x1 = "received/X1.csv";
			csv_file.open(output_x1);

			struct timeval start, end; // https://www.geeksforgeeks.org/measure-execution-time-with-high-precision-in-c-c/
			gettimeofday(&start, NULL); // start the timer
			
			for(int i=0; i<1000; i++){
				chan.cwrite(&d1, sizeof(d1));
				chan.cread (&reply1, sizeof(double));
				csv_file << d1.seconds<<"," << reply1 <<","; // output when ecgvalue is 1
				
				chan.cwrite(&d2, sizeof(d2));
				chan.cread (&reply1, sizeof(double));
				csv_file <<reply1 <<endl; // output when ecgvalue is 2

				d1.seconds = d1.seconds +0.004; // the time is increment by 4 mili seconds
				d2.seconds = d2.seconds +0.004;
			}
			gettimeofday(&end, NULL);// end the timer

			double total_time;
			total_time = (end.tv_sec - start.tv_sec)+(end.tv_usec - start.tv_usec)* 1e-6 ; // seconds + microseconds
			cout << "Time taken to ouput x1.csv is: " << fixed << total_time <<" sec" << endl;
			cout<<endl;
		}
		
		
	}///////////////////////
		else if(filereq){ // ask for file request
		FileRequest fm (0,0);
		int len = sizeof (FileRequest) + filename.size()+1;
		char buf2 [len];
		memcpy (buf2, &fm, sizeof (FileRequest));
		strcpy (buf2 + sizeof (FileRequest), filename.c_str());
		chan.cwrite (buf2, len);  
		int64 filelen;
		chan.cread (&filelen, sizeof(int64));
		if (isValidResponse(&filelen)){
			cout << "\nFile length is: " << filelen << " bytes" << endl;
		}


		//////////// editing
		FileRequest *filemessage = (FileRequest*) buf2;
		string output_path =  "received/" + filename;
		FILE* output_file = fopen(output_path.c_str(),"w"); // write to the file
		filemessage->offset = 0;
		char* buf_receive = new char[buffercapacity];
		
		struct timeval start1, end1;// start counting the time
		gettimeofday(&start1, NULL);
		int remain_len = filelen;

		while (remain_len>0){
			filemessage->length =  min( (int) remain_len,  buffercapacity);
			chan.cwrite(buf2, len);
			chan.cread(buf_receive,buffercapacity);
			fwrite(buf_receive, 1, filemessage->length, output_file);
			filemessage->offset+=filemessage->length;
			remain_len = remain_len - filemessage->length;
		}
		gettimeofday(&end1, NULL);// end the timer

		double total_time1;
		total_time1 = (end1.tv_sec - start1.tv_sec)+(end1.tv_usec - start1.tv_usec)* 1e-6 ; // seconds + microseconds
		cout << "Time taken transfer file is: " << fixed << total_time1 <<" sec" << endl;
		cout<< "Finished File Transfer!!!"<<endl;
		cout<<endl;
	}
	}
	
	

	
	/* this section shows how to get the length of a file
	you have to obtain the entire file over multiple requests 
	(i.e., due to buffer space limitation) and assemble it
	such that it is identical to the original*/
	
	
	



	

	// closing the channel    
    Request q (QUIT_REQ_TYPE);
    chan.cwrite (&q, sizeof (Request));
	// client waiting for the server process, which is the child, to terminate
	wait(0);
	cout << "Client process exited" << endl;

}
