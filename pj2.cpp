#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <list>
#include <algorithm>

using namespace std;

struct Process {
    string name; //프로세스 이름
    int pid; //프로세스 ID
    int ppid; //부모 프로세스 ID
    char waiting_type; ////waiting일 때 S, W중 하나
    int sleep_remain_counter;
};

struct Status {
    int cycle;
    string mode;
    string command;
    Process* process_running;
    vector<Process*> process_ready;
    vector<Process*> process_waiting;
    Process* process_new;
    Process* process_terminated;  
};

// 전역변수
int pid_decider = 0;
FILE* file;

// 출력 함수
void print_status(Status status) {

    //0. 몇번째 cycle인지
        fprintf(file, "[cycle #%d]\n", status.cycle);
    
    //1. 현재 실행 모드 (user or kernel)
        fprintf(file, "1. mode: %s\n", status.mode.c_str());

    //2. 현재 실행 명령어
        fprintf(file, "2. command: %s\n", status.command.c_str());

    //3. 현재 실행중인 프로세스의 정보. 없을 시 none 출력
    if (status.process_running==NULL) {
        fprintf(file, "3. running: none\n");
    } else {
        Process* i = status.process_running;
        fprintf(file, "3. running: %d(%s, %d)\n", i->pid, i->name.c_str(), i->ppid);
    }

    //4. 현재 ready 상태인 프로세스의 정보. 왼쪽에 있을 수록 먼저 queue에 들어온 프로세스이다.
    if (status.process_ready.size() ==0) {
        fprintf(file, "4. ready: none\n");
    } else {
        fprintf(file, "4. ready:");
        for(int i =0; i < status.process_ready.size(); ++i) {//공백 한칸으로 구분
            fprintf(file, "%d", status.process_ready[i]->pid);
        }
        fprintf(file, "\n");
    }

    //5. 현재 waiting 상태인 프로세스의 정보. 왼쪽에 있을 수록 먼저 waiting이 된 프로세스이다.
    if (status.process_waiting.size() ==0) {
        fprintf(file, "5. waiting: none\n");
    } else {
        fprintf(file, "5. waiting:");
        for(int i =0; i < status.process_waiting.size(); ++i) {//공백 한칸으로 구분
            fprintf(file, "%d(%c)", status.process_waiting[i]->pid, status.process_waiting[i]->waiting_type);
        }
        fprintf(file, "\n");
    }

    //6. 현재 new 상태인 프로세스의 정보. 없을 시 none 출력
    if (status.process_new==NULL) {
        fprintf(file, "6. new: none\n");
    } else {
        Process* i = status.process_new;
        fprintf(file, "6. new: %d(%s, %d)\n", i->pid, i->name.c_str(), i->ppid);
    }

    //7. 현재 terminated 상태인 프로세스의 정보. 없을 시 none 출력
    if (status.process_terminated==NULL) {
        fprintf(file, "7. terminated: none\n");
    } else {
        Process* i = status.process_terminated;
        fprintf(file, "7. terminated: %d(%s, %d)\n", i->pid, i->name.c_str(), i->ppid);
    }

    fprintf(file, "\n");
}



//프로세스를 생성하는 함수
Process* create_process(string name, int pid, int ppid) {
    Process* process = new Process;
    process->name = name;
    process->pid = pid;
    process->ppid = ppid;
    pid_decider++;
    return process;
}

//프로세스를 ready queue에 넣는 함수
void add_process_to_ready_queue(Process* process, Status* status) {
    status->process_ready.push_back(process);
}

//프로세스를 waiting queue에 넣는 함수
void add_process_to_waiting_queue(Process* process, Status* status) {
    status->process_waiting.push_back(process);
}

//프로세스를 terminated queue에 넣는 함수
void add_process_to_terminated_queue(Process* process, Status* status) {
    status->process_terminated = process;
}

//프로세스를 running queue에 넣는 함수
void add_process_to_running_queue(Process* process, Status* status) {
    status->process_running = process;
}

//프로세스를 new queue에 넣는 함수
void add_process_to_new_queue(Process* process, Status* status) {
    status->process_new = process;
}

//프로세스를 ready queue에서 제거하는 함수
void remove_process_from_ready_queue(Process* process, Status* status) {
    for (int i = 0; i < status->process_ready.size(); ++i) {
        if (status->process_ready[i]->pid == process->pid) {
            status->process_ready.erase(status->process_ready.begin() + i);
            break;
        }
    }
}

//프로세스를 waiting queue에서 제거하는 함수
void remove_process_from_waiting_queue(Process* process, Status* status) {
    for (int i = 0; i < status->process_waiting.size(); ++i) {
        if (status->process_waiting[i]->pid == process->pid) {
            status->process_waiting.erase(status->process_waiting.begin() + i);
            break;
        }
    }
}

//프로세스를 terminated queue에서 제거하는 함수
void remove_process_from_terminated_queue(Process* process, Status* status) {
    status->process_terminated = NULL;
}

//프로세스를 running queue에서 제거하는 함수
void remove_process_from_running_queue(Process* process, Status* status) {
    status->process_running = NULL;
}

//프로세스를 new queue에서 제거하는 함수
void remove_process_from_new_queue(Process* process, Status* status) {
    status->process_new = NULL;
}


//프로세스를 running queue에서 제거하고 terminated queue에 넣는 함수
void move_process_from_run_to_terminated_queue(Process* process, Status* status) {
    remove_process_from_running_queue(process, status);
    add_process_to_terminated_queue(process, status);
}

//프로세스를 new queue에서 제거하고 running queue에 넣는 함수
void move_process_from_new_to_running_queue(Process* process, Status* status) {
    remove_process_from_new_queue(process, status);
    add_process_to_running_queue(process, status);
}

//프로세스를 running queue에서 제거하고 ready queue에 넣는 함수
void move_process_from_run_to_ready_queue(Process* process, Status* status) {
    remove_process_from_running_queue(process, status);
    add_process_to_ready_queue(process, status);
}

//프로세스를 ready queue에서 제거하고 running queue에 넣는 함수
void move_process_from_ready_to_running_queue(Process* process, Status* status) {
    remove_process_from_ready_queue(process, status);
    add_process_to_running_queue(process, status);
}

//프로세스를 new queue에서 제거하고 ready queue에 넣는 함수
void move_process_from_new_to_ready_queue(Process* process, Status* status) {
    remove_process_from_new_queue(process, status);
    add_process_to_ready_queue(process, status);
}

//프로세스를 running queue에서 제거하고 waiting queue에 넣는 함수
void move_process_from_run_to_waiting_queue(Process* process, Status* status) {
    remove_process_from_running_queue(process, status);
    add_process_to_waiting_queue(process, status);
}

//프로세스를 waiting queue에서 제거하고 ready queue에 넣는 함수
void move_process_from_wait_to_ready_queue(Process* process, Status* status) {
    remove_process_from_waiting_queue(process, status);
    add_process_to_ready_queue(process, status);
}


//cycle 증가 구현 함수
void increase_cycle(Status* status) {
    status->cycle++;
    for (int i = 0; i < status->process_waiting.size(); i++) {
        if (status->process_waiting[i]->waiting_type == 'S') {
            status->process_waiting[i]->sleep_remain_counter--;
            if (status->process_waiting[i]->sleep_remain_counter == 0) {
                move_process_from_wait_to_ready_queue(status->process_waiting[i], status);
            }
        }
    }
}



//명령어 목록
//1. run arg1
//  arg1로 주어진 cycle만큼 해당 프로세스를 실행한다.
//2. sleep arg1
//  arg1로 주어진 cycle만큼 프로세스 상태가 waiting으로 바뀐다.
//  모드를 kernel mode로 바꾼다.
//3. fork_and_exec arg1
//  arg1로 주어진 파일 이름을 자식 프로세스로 생성하고 ready queue에 넣는다.
//  자식 프로세스의 이름은 arg1로 주어진 파일 이름이다.
//  자식 프로세스는 부모 프로세스의 pid를 ppid로 가진다.
//  자식 프로세스의 pid는 마지막으로 생성된 프로세스의 pid + 1이다.
//  모드를 kernel mode로 바꾼다.
//4. wait
//  임의의 자식 프로세스 중에서 exit이 발생할 때까지 대기한다.
//  프로세스 상태가 waiting이 되어 대기한다.
//  종료되지 않은 자식 프로세스가 존재한다면, 임의의 자식 프로세스의 exit 처리 과정에서 ready queue에 넣는다.
//  모드를 kernel mode로 바꾼다.
//5. exit
//  프로세스 상태가 terminated가 된다.
//  모드를 kernel mode로 바꾼다.

//명령어를 실행하는 함수
void run_command(string command, Status* status) {
    vector<string> command_list;
    size_t start = 0, end = 0;
    while ((end = command.find(' ', start)) != string::npos) { // ' ' 문자열을 기준으로 문자열 분할
        command_list.push_back(command.substr(start, end - start)); // 문자열 분할 결과를 command_list에 추가
        start = end + 1; // 다음 문자열 시작 위치로 이동
    }
    command_list.push_back(command.substr(start)); // 마지막 문자열 추가

    string command_name = command_list[0];
    string arg1;

    if (command_list.size() > 1) {
        arg1 = command_list[1];
    } else {
        arg1 = "";
    }

    

    if (command_name == "run") {
        increase_cycle(status);

        status->mode = "user";
        status->command = "run";

        for (int i = 0; i < stoi(arg1); i++) {
            increase_cycle(status);
        }
        
    } else if (command_name == "sleep") {
        increase_cycle(status);

        status->mode = "user";
        status->command = "sleep";

        increase_cycle(status);

        status->mode = "kernel";
        status->command = "system call";
        status->process_running->waiting_type = 'S';
        status->process_running->sleep_remain_counter = stoi(arg1);
        move_process_from_run_to_waiting_queue(status->process_running, status);

        increase_cycle(status);

        while (status->process_ready[0] == NULL) {
            status->command = "idle";
            increase_cycle(status);
        }
        status->command = "schedule";
        move_process_from_ready_to_running_queue(status->process_ready[0], status);


    } else if (command_name == "fork_and_exec") {
        increase_cycle(status);

        status->mode = "user";
        status->command = "fork_and_exec";

        increase_cycle(status);

        status->mode = "kernel";
        status->command = "system call";
        move_process_from_run_to_ready_queue(status->process_running, status);
        Process* process = create_process(arg1, pid_decider, status->process_running->pid);
        add_process_to_new_queue(process, status);

        increase_cycle(status);

        while (status->process_ready[0] == NULL) {
            status->command = "idle";
            increase_cycle(status);
        }
        status->command = "schedule";
        move_process_from_ready_to_running_queue(status->process_ready[0], status);
        move_process_from_new_to_ready_queue(process, status);

        

    } else if (command_name == "wait") {
        increase_cycle(status);

        status->mode = "user";
        status->command = "wait";

        increase_cycle(status);

        status->mode = "kernel";
        status->command = "system call";
        status->process_running->waiting_type = 'W';
        move_process_from_run_to_waiting_queue(status->process_running, status);

        increase_cycle(status);

        while (status->process_ready[0] == NULL) {
            status->command = "idle";
            increase_cycle(status);
        }
        status->command = "schedule";
        move_process_from_ready_to_running_queue(status->process_ready[0], status);


        

    } else if (command_name == "exit") {
        increase_cycle(status);

        status->mode = "user";
        status->command = "exit";

        increase_cycle(status);

        status->mode = "kernel";
        status->command = "system call";
        move_process_from_run_to_terminated_queue(status->process_running, status);

        for (int i = 0; i < status->process_waiting.size(); i++) {
            if (status->process_waiting[i]->waiting_type == 'W' && status->process_waiting[i]->ppid == status->process_terminated->pid) {
                move_process_from_wait_to_ready_queue(status->process_waiting[i], status);
            }
        }

        increase_cycle(status);

        remove_process_from_terminated_queue(status->process_terminated, status);
        if (status->process_waiting[0] == NULL && status->process_ready[0] == NULL && status->process_new == NULL) {
            }
        else if (status->process_ready[0] == NULL) {
            while (status->process_ready[0] == NULL) {
                status->command = "idle";
                increase_cycle(status);
            }
        } else {
            status->command = "schedule";
            move_process_from_ready_to_running_queue(status->process_ready[0], status);
        }
        status->command = "schedule";
        move_process_from_ready_to_running_queue(status->process_ready[0], status);
        
    }
}





int main(void) {
    
    file = fopen("result", "a");

    Status* status = new Status();
    status->cycle = 0;
    status->mode = "kernel";
    status->command = "";
    status->process_running = NULL;
    status->process_ready = {};
    status->process_waiting = {};
    status->process_new = NULL;
    status->process_terminated = NULL;

    Process* process = create_process("init", 1, 0);
    
   
    while (status->cycle < 10) {
        print_status(*status);
    }
    
    delete status;
    fclose(file);

    return 0;
}
