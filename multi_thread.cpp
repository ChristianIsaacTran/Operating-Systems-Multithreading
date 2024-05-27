/*
# =====================================================================
# Title             : multi_thread.cpp
# Description       : This program uses two concurrent threads to perform reading from a file, and processing the data into different files by writing with fstream.
# Author            : Christian Tran (R11641653)
# Date              : 3/20/24
# Usage             : With the Makefile and sbatch command in HPCC
# Notes             : requires HPCC and sbatch command
=====================================================================
*/ 
#include <iostream> 
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <fstream> 
#include <math.h>

using namespace std; //Import namespace to not have to call std every time.

const int max_args = 2; //Max number of arguments allowed from command line
const int integerQ_size = 50; //Max size of the queue (buffer size)
queue<int> integerQ; //queue to control the producer read data (buffer itself)
mutex mtx; //mutex lock to protect the queue between the producer and consumer
condition_variable condV_reader; //condition variable to communicate to decision thread
condition_variable condV_decision; //condition variable to communicate to reader thread


//use ofstream to create the output files for each category 
ofstream even("even.out");
ofstream odd("odd.out");
ofstream pos("positive.out");
ofstream neg("negative.out");
ofstream squ("square.out");
ofstream cube("cube.out");


//reader thread used to read the integer inputs from file. (producer)
/*
note to self: in this program, our shared resource that we want to protect is going to be the integerQ queue, so we use mutex locks to prevent modification by more than one thread.
*/
void reader(char* fileN){

    //Create fstream to read file argument and open it
    ifstream inputfile_arg;
    inputfile_arg.open(fileN);

    if(inputfile_arg.is_open()){  //Check to see if file is open or not/if it exists
        int line_num; //Represents every individual line in a file that has an int
        while(inputfile_arg >> line_num){ //gets every line from the input file until the EOF
            unique_lock<mutex> lock(mtx); //gets mutex lock
            while (integerQ.size() == integerQ_size){ //if the buffer queue is full, then tell this thread to wait with condition variable
                condV_reader.wait(lock); //automatically calls unlock when waiting for other threads, and locks when it "wakes up" 
            }
            integerQ.push(line_num); //Produces the data from the file (int) and pushes it into our buffer

            lock.unlock(); //unlocks the mutex lock to share the buffer to the consumer when it wakes up 

            condV_decision.notify_one(); //wakes up the decision_maker thread to start "consuming" data and making decisions after producing
        }

        exit(0); //Exit program after reaching EOF 

    }
    else{ //error message and exit program if the file is not open, or not found
        cout << "ERROR: File not found or invalid argument/filename. Terminating..." << endl; 
        exit(0); //Exits program 
    }
}


//decision_maker thread to make decisions on each integer in the queue, and then appends them to their appropriate file classification (consumer)
void decision_maker(){

    while(true){ //while loop to consume data forever until it has to wait (consumes all the data in buffer)
        unique_lock<mutex> lock(mtx); //gets mutex lock
        while(integerQ.empty()){ //if the integerQ (buffer) is empty, wait for reader to read more data and fill buffer
            condV_decision.wait(lock);
        }
        
        int current_num = integerQ.front(); //gets the first number from the queue
        integerQ.pop(); //removes the first element we just parsed, "consuming" it
        
        //Even or odd
        if(current_num % 2 == 0){ //Check if number is even, write to even file
            even << current_num << endl; 
        }
        else{ //Check if number is odd, write to odd file
            odd << current_num << endl;
        }

        //Positive or negative, exclude zero
        if(current_num > 0){ //Check if number is positive, write to pos file
            pos << current_num << endl;
        }
        if(current_num < 0){ //Check if number is negative, write to neg file, exclude zero
            neg << current_num << endl;
        }

        //square root
        int sq_root = sqrt(current_num); //Get square root 

        if(current_num >= 0){ //Square root has to be positive, or zero. 
            if(sq_root * sq_root == current_num){ //Check if perfect square, write to squ file
                squ << current_num << endl;
            }
        }

        //Cube root
        int cb_root = round(cbrt(current_num)); //Get cube root

        if(cb_root * cb_root * cb_root == current_num){ //Check if number is a perfect cube, write to cube file
            cube << current_num << endl;
        }

        lock.unlock(); //Unlock the mutex whenever this consumes data so that we can wake up producer to produce more

        condV_reader.notify_one(); //Wake up reader to read more data if there is space available in integerQ queue

    }


}

/*main function has arguments (int argc, char* argv[])
argc = count of the amount of arguments called with this C++ program 
argv = char pointer array that contains all of the arguments in each cell 
        - note: argv[0] always contains the name of the C++ program itself
*/
int main(int argc, char* argv[]){ 
//Required that threads need to run concurrently, no linear. Use .join() to make sure that the code waits for all the threads to finish before killing the program.

    //error Check for max number of arguments with error message and termination of program
    if(argc > max_args || argc == 1){
        cout << "ERROR: Not enough arguments or too many arguments found. Terminating program..." << endl;
        return 0; 
    }

    //pull file name from argv (arguments) list and pass to reader thread to open and read file:
    char* fileN = argv[1]; 

    
    //Spawn both threads to run concurrently:
    thread read_thread(reader, fileN);
    thread decide_thread(decision_maker);

    //Make the program wait for the threads to finish before killing itself:
    read_thread.join();
    decide_thread.join();

    return 0; 
}