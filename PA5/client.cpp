#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "TCPRequestChannel.h"
#include <time.h>
#include <thread>
using namespace std;


struct Response{
    int person;
    double ecg;
};
void timediff(struct timeval& start, struct timeval& end){
    // int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    // int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    double total_time;
    total_time = (end.tv_sec - start.tv_sec)+(end.tv_usec - start.tv_usec)* 1e-6 ; // seconds + microseconds
    cout << "Took " << fixed << total_time <<" sec" << endl;
    //cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

}


void patient_thread_function(int n, int pno, BoundedBuffer* request__buffer){
    datamsg dx(pno, 0.0,1);
    for(int i = 0; i < n; i++)
    {
        request__buffer->push((char*)&dx, sizeof(datamsg));
        dx.seconds += 0.004;


    }
    /* What will the patient threads do? */
}

void worker_thread_function(TCPRequestChannel* chan, BoundedBuffer* requestbuffer, BoundedBuffer *responseBuffer, int mb){
    char buf[1024];
    double resp = 0;

    char receivebuf[mb];
    while(true)
    {
        requestbuffer->pop(buf,1024);
        MESSAGE_TYPE*m = (MESSAGE_TYPE*) buf;

        if(*m == DATA_MSG)
        {
            datamsg * d = (datamsg *) buf;
            chan->cwrite(d,sizeof(datamsg));
            double ecg;
            chan -> cread(&ecg, sizeof(double));
            Response r{d->person, ecg};
            responseBuffer->push((char *) &r, sizeof(r));
        }

        else if(*m ==QUIT_MSG)
        {
            chan->cwrite(m, sizeof(MESSAGE_TYPE));
            delete chan;
            break;

        }
        else if(*m ==FILE_MSG)
        {
            filemsg* fm = (filemsg*) buf;
            string fname = (char*) (fm+1);
            int siz = sizeof(filemsg)+ fname.size()+1;
            chan->cwrite(buf, siz);
            chan->cread(receivebuf, mb);

            string recvfilename = "recv/" + fname;

            FILE* fp = fopen(recvfilename.c_str(),"r+"); //both read and write mode, expects the file to be there already
            fseek(fp,fm->offset , SEEK_SET);
            fwrite(receivebuf,1,fm->length, fp);
            fclose(fp);


        }
    }
}

void histogram_thread_function (BoundedBuffer * response_Buffer, HistogramCollection *hx){
   char buff[1024];
   while(true){
       response_Buffer->pop(buff,1024);

       Response *r = (Response *) buff;
       if(r->person == -1){
           break;
        }
        else{
           hx->update(r->person, r->ecg);
       }
    }
}

void file_thread_function(string filename, BoundedBuffer* request_buffer,TCPRequestChannel* chan, int mb)
{
    if(filename == "")//means we're not doing a file transfer
        return;
    //1. Create the file
    string recvfilename = "recv/"+filename;

    //make it as long as the original length
    char buf[1024];
    filemsg f(0,0);
    memcpy(buf, &f, sizeof(f));
    strcpy(buf+sizeof(f), filename.c_str());
    chan->cwrite(buf, sizeof(f)+filename.size()+1);//+1 because of null byte
    u_int64_t filelength;
    chan->cread(&filelength, sizeof(filelength));

    //creating the file
    FILE* fp = fopen(recvfilename.c_str(), "w");
    fseek(fp, filelength, SEEK_SET);//making the file of length filelength
    fclose(fp);

    filemsg* fx = (filemsg*) buf;
    u_int64_t remlength = filelength;

    while(remlength>0)
    {
        fx->length = min(remlength,(u_int64_t) mb);
        request_buffer->push(buf, sizeof(filemsg)+filename.size()+1);
        fx->offset +=fx->length;
        remlength -= fx->length;
    }
}



int main(int argc, char *argv[])
{
    int n = 15000;    //default number of requests 
    int p = 15;     // number of patients 
    int w = 200;    //default number of worker threads
    int b = 1024; 	// default capacity of the request buffer
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    int h = 10; // histogram
    srand(time_t(NULL));
    string filename = "a.dat";//hardcoded file name
    bool istransfer_file = false;
    string host, port;

    int opt = -1;
    while((opt = getopt(argc,argv,"m:n:b:w:p:f:h:r:"))!=-1)
    {
        switch (opt)
        {
            case 'm':
                m = atoi(optarg);
                break;
            case 'n':
                n = atoi(optarg);
                break;
            case 'p':
                p = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'f':
                filename = optarg;
                istransfer_file = true;
                break;
            case 'h':
                host = optarg;// host name
                break;
            case 'r':
                port = optarg; // port number
                break;
        }
    }
    
    
    // int pid = fork();
    // if (pid == 0){
	// 	// modify this to pass along m
    //     char *args[] = {"./server", "-m", (char *) to_string(m).c_str(),
    //                     NULL};
    //     if (execvp(args[0], args) < 0) {
    //         perror("exec filed");
    //         exit(0); 
    //     }
    //     //execl ("server", "server", (char *)NULL);
    // }
    
	TCPRequestChannel* chan = new TCPRequestChannel(host,port);
    BoundedBuffer requestbuffer(b);
    BoundedBuffer responseBuffer(b);
	HistogramCollection hc;

	//making histograms

	for(int i = 0; i < p; i++)
    {
	    hc.add(new Histogram(10,-2.0,2.0));
    }

	//make worker channels
	TCPRequestChannel* wchanel [w];
	for(int i = 0; i < w;i++)
    {
	    wchanel[i] = new TCPRequestChannel (host, port);
    }
	
	
	
    struct timeval start, end;
    gettimeofday (&start, 0);

    /* Start all threads here */
    thread patient [p];
    if(!istransfer_file)
    {
        //thread patient [p];
        for(int i = 0; i < p;i++)
        {
            patient[i] = thread(patient_thread_function,n,i+1, &requestbuffer );
        }
       
    }
    
    thread filethread(file_thread_function, filename, &requestbuffer, chan, m);
    


    thread workers[w];
    for(int i = 0; i < w;i++)
    {
        workers[i] = thread(worker_thread_function, wchanel[i],&requestbuffer, &responseBuffer,m);
    }


    thread hists [h];
	if(!istransfer_file){
        for(int i = 0; i < h; i++){
            hists[i] = thread(histogram_thread_function, &responseBuffer, &hc);
        }
    }

//	/* Join all threads here */
	if(!istransfer_file)
    {
        for(int i = 0; i < p;i++)
        {
            patient[i].join();
        }
    }

	filethread.join();
    

    for(int i = 0; i < w;i++)
    {
        MESSAGE_TYPE q = QUIT_MSG;
        requestbuffer.push ((char*) &q,sizeof(q));
    }

    for(int i = 0; i < w;i++)
    {
        workers[i].join();
    }
    

    Response r{-1,1};
    for(int i = 0; i < h; i++){
        responseBuffer.push((char *)&r, sizeof(r));
    }

    if(!istransfer_file){
        for(int i = 0; i < h; i++){
            hists[i].join();
        }
    }

    gettimeofday (&end, 0);

    // print the results
	hc.print ();

    timediff(start, end);
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    
}
