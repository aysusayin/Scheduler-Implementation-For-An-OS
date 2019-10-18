#include <iostream>
#include <vector>
#include <fstream>
#include <iterator>
#include <list>
#include <sstream>
#include <algorithm>
#include <queue>

using namespace std;

struct Code {
    string name; // Name of the code file
    int size; // Number of instructions
    vector<int> instructionList; // Vector that stores executing time of instructions
};

struct Process {
    int processNo;
    int priority;
    int arrivalTime;
    int exitTime;
    int waitTime;
    Code *code;
    int lastProcessedTime; // Last time when this process is processed in CPU
    int instructionIndex; // Index of the next instruction to be executed
};

// To compare processes according to their priority in the ready queue, Comparision class is needed
class Comparison {
public:
    bool operator()(const Process *p1, const Process *p2) const {
        int val = p1->priority - p2->priority;
        if (val != 0)
            return p1->priority > p2->priority;
        else
            return p1->arrivalTime > p2->arrivalTime; // If their priorities are equal FCFS
    }
};

typedef std::priority_queue<Process *, std::vector<Process *>, Comparison> pq_type; // Special type definition for the priority queue which uses Comparison class as comparator

// This method returns the index of the element whose name is given as parameter from the vector which is given as parameter.
int indexOf(string name, vector<string> &vec) {
    auto it = find(vec.begin(), vec.end(), name);
    if (it != vec.end()) {
        return it - vec.begin();
    } else {
        cout << "Code File is not given.";
        return -1;
    }
}

// Splits the given string into tokens and stores in the container which is given
template<class Container>
void split(const string &str, Container &cont) {
    istringstream iss(str);
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter(cont));
}

// Prints the elements in the priority queue at the given time
string printQueue(int time, pq_type pq) {
    string output;
    output = to_string(time) + ":HEAD-";
    if (pq.empty()) output = output + "-";
    while (!pq.empty()) {
        output = output + "P" + to_string(pq.top()->processNo + 1) + "[" + to_string(pq.top()->instructionIndex + 1) +
                 "]-";
        pq.pop();
    }
    output = output + "TAIL\n";
    return output;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cout << "Process Definition and programming code files are required.";
        return 1;
    }
    vector<string> codeFiles; // Stores the names of the code files
    vector<Code> codes; // Stores the codes
    vector<Process> processes; // Stores the processes

    // Gets the instructions in the code files and stores them in codes vector.
    for (int i = 2; i < argc; i++) {
        codeFiles.push_back(argv[i]);
        ifstream inCodeFile;
        inCodeFile.open(argv[i]);
        string line;
        Code c;
        c.name = argv[i];
        while (getline(inCodeFile, line)) {
            vector<string> words;
            split(line, words);
            c.instructionList.push_back(stoi(words[1]));
        }
        inCodeFile.close();
        c.size = c.instructionList.size();
        codes.push_back(c);
    }

    // Gets the processes from process file and stores them in process vector.
    ifstream processFile(argv[1]);
    string line;
    while (processFile.is_open() && getline(processFile, line)) {
        vector<string> words;
        split(line, words);
        Process p;
        p.priority = stoi(words[1]);
        int indexOfCodeFile = indexOf(words[2] + ".txt", codeFiles);
        p.code = &codes[indexOfCodeFile];
        p.instructionIndex = 0;
        p.arrivalTime = stoi(words[3]);
        p.exitTime = -1;
        p.waitTime = 0;
        p.lastProcessedTime = stoi(words[3]);
        p.processNo = processes.size();
        processes.push_back(p);
    }
    processFile.close();

    // Creates output file.
    ofstream outputFile;
    outputFile.open("output.txt");

    //Scheduling

    pq_type readyQueue;
    Process *currentProcess; // Process in the CPU
    int lastNewProcessIndex = 0; // Index of the last process that is added to the ready queue
    int currentTime = processes[0].arrivalTime;

    if (currentTime != 0) {
        // Print the queue at time 0
        outputFile << printQueue(0, readyQueue);
    }
    // Push the first process to the ready queue
    readyQueue.push(&processes[0]);

    // If multiple processes come at the first arrival time, push them to ready queue
    while (currentTime == processes[lastNewProcessIndex + 1].arrivalTime) {
        lastNewProcessIndex++;
        readyQueue.push(&processes[lastNewProcessIndex]);
    }

    while (!readyQueue.empty()) {
        // Process the head of the ready queue
        currentProcess = readyQueue.top();
        // Print the queue
        outputFile << printQueue(currentTime, readyQueue);
        // Set the waiting time for the current process
        currentProcess->waitTime += currentTime - currentProcess->lastProcessedTime;
        bool check = true; // True if we are continuing with the same process; False otherwise
        bool is_finished = false; // True  if the current process has no instructions left to execute; False otherwise
        while (check) {
            // Execute the next instruction of the current process
            currentTime += currentProcess->code->instructionList[currentProcess->instructionIndex];
            bool isCurrentProcessPopped = false;
            if (currentProcess->code->size - 1 > currentProcess->instructionIndex) {
                // Current process is not finished so increment the instruction index
                currentProcess->instructionIndex++;
            } else {
                // Current Process is finished
                currentProcess->exitTime = currentTime;
                check = false;
                is_finished = true;
                readyQueue.pop();
                isCurrentProcessPopped = true;
            }

            int checkPrint = -1; // This is for checking if we need to print the ready queue
            // Check if a new process has arrived
            while (lastNewProcessIndex < processes.size() - 1 &&
                   processes[lastNewProcessIndex + 1].arrivalTime <= currentTime) {
                lastNewProcessIndex++;
                if (processes[lastNewProcessIndex].priority < currentProcess->priority) {
                    // The priority of the new process is higher than the current process
                    if (!isCurrentProcessPopped) {
                        readyQueue.pop();
                        // Save status of current process and put it back to ready queue
                        currentProcess->lastProcessedTime = currentTime;
                        readyQueue.push(currentProcess);
                        isCurrentProcessPopped = true;
                    }
                    check = false;
                    checkPrint = 1;
                } else {
                    // If we changed the current process, don't need to print it below since it will be printed
                    // when the new process is popped from the ready queue
                    if (checkPrint != 1) checkPrint = 0;
                }
                readyQueue.push(&processes[lastNewProcessIndex]);
            }
            if (!is_finished && checkPrint == 0) {
                // Print the queue
                outputFile << printQueue(currentTime, readyQueue);
            }
            // Check if the ready queue is empty and if the new processes are available
            if (is_finished && readyQueue.empty() && lastNewProcessIndex < processes.size() - 1) {
                outputFile << printQueue(currentTime, readyQueue);
                lastNewProcessIndex++;
                readyQueue.push(&processes[lastNewProcessIndex]);
                currentTime = processes[lastNewProcessIndex].arrivalTime;
            }
        }
    }
    outputFile << printQueue(currentTime, readyQueue) << "\n";
    // Print the turnaround time and waiting time
    for (int i = 0; i < processes.size(); i++) {
        Process p = processes[i];
        int turnaroundTime = p.exitTime - p.arrivalTime;
        outputFile << "Turnaround time for P" << p.processNo + 1 << " = " << turnaroundTime << " ms\n";
        outputFile << "Waiting time for P" << p.processNo + 1 << " = " << p.waitTime;
        if (i != processes.size() - 1) outputFile << "\n";
    }
    outputFile.close();
    return 0;
}