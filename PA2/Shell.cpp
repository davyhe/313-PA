#include<iostream>
#include<string>
#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<unistd.h>
#include<fcntl.h>
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include<ctime>
#include<unistd.h>
#include <string.h>

using namespace std;

string trim(string input){
    int i=0;
    while(i<input.size() && input[i]==' '){
        i++;
    }
    if(i<input.size())
        input = input.substr(i);
    else{
        return "";
    }
    i=input.size() -1;
    while(i>=0 && input[i]==' ')
        i--;
    if(i>=0){
        input = input.substr(0, i+1);
    }
    else{
        return "";
    }
    return input;
}


char** vec_to_char_array (vector<string>& parts){
    char** result= new char *[parts.size()+1]; // add 1 for the NULL pointer at the end
    for(int i=0; i<parts.size();i++){
        result [i] = (char*) parts[i].c_str();
    }
    result [parts.size()] = NULL;
    return result;
}

vector<string> split (string line, char separator=' '){
    int count=0;
    int count_quotes =0; // count number of quotes
    vector<string> input;
    int begin =0;
    bool inquote = false;
    
    for(int i = 0; i < line.size(); i++){

        if ((line[i] == '\'' || line[i] == '\"') && separator == ' ' ){
            string single_word;
            count_quotes++;
            if (count_quotes %2 !=0){ // count all the quote
                begin = i+1;
                inquote = true;
            }
            else {
                inquote = false;
                for (auto k = begin; k < i; k++){
                    single_word.push_back(line[k]);
                }
                input.push_back(single_word);
            }
        }
       else if (inquote == false){
            if((i == (line.size()-1)) || (line[i] == separator) ){
                string single_word;
                if (i == line.size() - 1){
                    for (auto k = count; k < i + 1; k++)
                        single_word.push_back(line[k]);
                }
                else{
                    for (auto k = count; k < i; k++)
                        single_word.push_back(line[k]);
                }
                input.push_back(single_word);
                count =i+1;
            }
        } // end of big else if
    }
    return input;
    
}

char* gettime(){ // https://www.tutorialspoint.com/print-system-time-in-cplusplus-3-different-ways
   time_t t; // t passed as argument in function time()
   time (&t); //passing argument to time()
   struct tm * tt; // decalring variable for localtime()
   tt = localtime(&t);
   return asctime(tt);
}



int main(){
    vector<int> bgs; // list of bgs
    while(true){
        for(int i=0; i<bgs.size();i++){
            if(waitpid(bgs[i], 0, WNOHANG) == bgs[i]){
                cout<<"Process: " <<bgs[i]<<"ended"<<endl;
                bgs.erase(bgs.begin()+i);
                i--;
            }
        }

        cout<<"Davy's Shell$  "<<gettime(); // print the prompt
        string inputline;
        getline(cin, inputline);
        if(inputline == string("exit")){
            cout<<"Bye!! End of Shell"<<endl;
            break;
        }

        bool bg = false;
        inputline = trim(inputline); // erase extra empty spaces
        if(inputline[inputline.size()-1] == '&'){
            //cout<<"Bg process found"<<endl;
            bg = true;
            inputline = inputline.substr(0, inputline.size()-1);
        }
        

        int pid = fork();
        

        if (pid == 0){
            vector<string> parts =split(inputline);
            char** args = vec_to_char_array(parts);
            //char* args [] = {(char* ) inputline.c_str(), NULL};
            execvp(args[0], args);
        }
        else{
            if(!bg)
                waitpid(pid, 0, 0);
            else{
                bgs.push_back(pid);
            }
        }
    }
}

/*
int main(){
    if(fork()==0){
        printf("Child PID: %d\n", getpid());
    }
    else{
        printf("Parent PID: %d\n", getpid());
        while(1);
    }
    return 0;
}*/