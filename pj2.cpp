#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <list>
#include <algorithm>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// 프로세스 구조체
struct Process {
    string name; //프로세스 이름
    int pid; //프로세스 ID
    int ppid; //부모 프로세스 ID
    char waiting_type; //waiting일 때 S, W중 하나
    int sleep_remain_counter; //sleep 명령어가 실행되면 sleep_remain_counter를 1씩 감소시킨다. 0이 되면 ready queue로 이동한다.
};

// status 구조체
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
int pid_decider = 1;
vector<int> bookmarks;
FILE* file;
string directory_path;

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
            fprintf(file, " ");
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
            fprintf(file, " ");
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

// 파일을 읽어서 내용과 명령어 수를 얻는 함수
pair<vector<string>, int> read_file(string file_name) {
    fs::path file_path = directory_path;
    file_path /= file_name;        
    ifstream file(file_path);
    vector<string> file_content;
    string line;
    int line_number = 0;
    if (file.is_open()) {
        while (getline(file, line)) {
            file_content.push_back(line);
            line_number++;
       }
        file.close();
    }
    return make_pair(file_content, line_number);
}


//프로세스를 생성하는 함수
Process* create_process(string name, int pid, int ppid, int next_command_index) {
    Process* process = new Process;
    process->name = name;
    process->pid = pid;
    process->ppid = ppid;
    pid_decider++;
    bookmarks.push_back(0);
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

    //sleeping process의 sleep_remain_counter를 감소시키고 0이 되면 ready queue로 이동  
    for (int i = 0; i < status->process_waiting.size(); i++) {
        if (status->process_waiting[i]->waiting_type == 'S') {
            status->process_waiting[i]->sleep_remain_counter--;
            if (status->process_waiting[i]->sleep_remain_counter == 0) {
                move_process_from_wait_to_ready_queue(status->process_waiting[i], status);                
            }
        }
    }

    status->cycle++;
    print_status(*status);
}



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

    //명령어에 인자가 있는 경우
    if (command_list.size() > 1) {
        arg1 = command_list[1];
    } else {
        arg1 = "";
    }

    //명령어가 run 일 때
    if (command_name == "run") {

        increase_cycle(status);

        //현재 모드 확인 후 kernel mode일 때 schedule
        if (status->mode == "user") {

        } else {

        status->command = "schedule";

        if (status->process_ready.empty()) {
            move_process_from_new_to_running_queue(status->process_new, status);
            } else {
            move_process_from_ready_to_running_queue(status->process_ready[0], status);
            if (status->process_new != NULL){
            move_process_from_new_to_ready_queue(status->process_new, status);
            }
        }

        if (status->process_terminated != NULL) {
            remove_process_from_terminated_queue(status->process_terminated, status);
        }

        increase_cycle(status);
        }

        //run 명령어 실행
        status->mode = "user";
        status->command = "run " + arg1;
        bookmarks[status->process_running->pid-1]++;

        for (int i = 0; i < stoi(arg1)-1; i++) {
            increase_cycle(status);
        }

        //idle 처리
        while (status->process_ready.empty() && status->process_running == NULL && status->process_new == NULL && !(status->process_waiting.empty())) {
            increase_cycle(status);
            status->command = "idle";
        }
        
        //다음 명령어 실행
        run_command(read_file(status->process_ready[0]->name).first[bookmarks[status->process_ready[0]->pid-1]], status);
        
    //명령어가 sleep 일 때
    } else if (command_name == "sleep") {
        
        increase_cycle(status);

        //현재 모드 확인 후 kernel mode일 때 schedule
        if (status->mode == "user") {

        } else {

        status->command = "schedule";

        if (status->process_ready.empty()) {
            move_process_from_new_to_running_queue(status->process_new, status);
        } else {
            move_process_from_ready_to_running_queue(status->process_ready[0], status);
            if (status->process_new != NULL){
            move_process_from_new_to_ready_queue(status->process_new, status);
            }
        }

        if (status->process_terminated != NULL) {
            remove_process_from_terminated_queue(status->process_terminated, status);
        }
        
        increase_cycle(status);

        }
        
        //sleep 명령어 실행
        status->mode = "user";
        status->command = "sleep " + arg1;
        bookmarks[status->process_running->pid-1]++;

        increase_cycle(status);

        status->mode = "kernel";
        status->command = "system call";
        status->process_running->waiting_type = 'S';
        status->process_running->sleep_remain_counter = stoi(arg1);
        move_process_from_run_to_waiting_queue(status->process_running, status);

        //idle 처리
        while (status->process_ready.empty() && status->process_running == NULL && status->process_new == NULL && !(status->process_waiting.empty())) {
            increase_cycle(status);
            status->command = "idle";
        }

        //다음 명령어 실행
        run_command(read_file(status->process_ready[0]->name).first[bookmarks[status->process_ready[0]->pid-1]], status);
        
    //명령어가 fork_and_exec 일 때
    } else if (command_name == "fork_and_exec") {

        increase_cycle(status);

        //현재 모드 확인 후 kernel mode일 때 schedule
        if (status->mode == "user"){

        } else {

        status->command = "schedule";

        if (status->process_ready.empty()) {
            move_process_from_new_to_running_queue(status->process_new, status);
        } else {
            move_process_from_ready_to_running_queue(status->process_ready[0], status);
            if (status->process_new != NULL){
            move_process_from_new_to_ready_queue(status->process_new, status);
            }
        }

        if (status->process_terminated != NULL) {
            remove_process_from_terminated_queue(status->process_terminated, status);
        }

        increase_cycle(status);
        }

        //fork_and_exec 명령어 실행
        status->mode = "user";
        status->command = "fork_and_exec";
        bookmarks[status->process_running->pid-1]++;
        increase_cycle(status);

        status->mode = "kernel";
        status->command = "system call";
        Process* process = create_process(arg1, pid_decider, status->process_running->pid, 0);
        move_process_from_run_to_ready_queue(status->process_running, status);
        add_process_to_new_queue(process, status);
        
        //idle 처리
        while (status->process_ready.empty() && status->process_running == NULL && status->process_new == NULL && !(status->process_waiting.empty())) {
            increase_cycle(status);
            status->command = "idle";
        }

        //다음 명령어 실행
        run_command(read_file(status->process_ready[0]->name).first[bookmarks[status->process_ready[0]->pid-1]], status);
        
    //명령어가 wait 일 때
    } else if (command_name == "wait") {

        increase_cycle(status);

        //현재 모드 확인 후 kernel mode일 때 schedule
        if (status->mode == "user") {

        } else {
        status->command = "schedule";

        if (status->process_ready.empty()) {
            move_process_from_new_to_running_queue(status->process_new, status);
        } else {
            move_process_from_ready_to_running_queue(status->process_ready[0], status);
            if (status->process_new != NULL){
            move_process_from_new_to_ready_queue(status->process_new, status);
            }
        }    

        if (status->process_terminated != NULL) {
            remove_process_from_terminated_queue(status->process_terminated, status);
        }

        increase_cycle(status);
        }

        //wait 명령어 실행
        status->mode = "user";
        status->command = "wait";
        bookmarks[status->process_running->pid-1]++;

        increase_cycle(status);

        status->mode = "kernel";
        status->command = "system call";
        status->process_running->waiting_type = 'W';
        move_process_from_run_to_waiting_queue(status->process_running, status);

        //idle 처리
        while (status->process_ready.empty() && status->process_running == NULL && status->process_new == NULL && !(status->process_waiting.empty())) {
            increase_cycle(status);
            status->command = "idle";
        }

        //다음 명령어 실행
        run_command(read_file(status->process_ready[0]->name).first[bookmarks[status->process_ready[0]->pid-1]], status);

    //명령어가 exit 일 때
    } else if (command_name == "exit") {

        increase_cycle(status);

        //현재 모드 확인 후 kernel mode일 때 schedule
        if (status->mode == "user") {

        } else {

        status->command = "schedule";
        

        if (status->process_ready.empty()) {
            move_process_from_new_to_running_queue(status->process_new, status);
        } else {
            move_process_from_ready_to_running_queue(status->process_ready[0], status);
            if (status->process_new != NULL){
            move_process_from_new_to_ready_queue(status->process_new, status);
            }
        }

        if (status->process_terminated != NULL) {
            remove_process_from_terminated_queue(status->process_terminated, status);
            }
        increase_cycle(status);
        
        }

        //exit 명령어 실행
        status->mode = "user";
        status->command = "exit";
        bookmarks[status->process_running->pid-1]++;

        increase_cycle(status);

        status->mode = "kernel";
        status->command = "system call";
        move_process_from_run_to_terminated_queue(status->process_running, status);

        for (int i = 0; i < status->process_waiting.size(); i++) {
            if (status->process_waiting[i]->waiting_type == 'W' && status->process_waiting[i]->pid == status->process_terminated->ppid) {
                move_process_from_wait_to_ready_queue(status->process_waiting[i], status);
            }
        }

        //idle 처리
        while (status->process_ready.empty() && status->process_running == NULL && status->process_new == NULL && !(status->process_waiting.empty())) {
            increase_cycle(status);
            status->command = "idle";
        }

        //idle 상태가 아니고, ready queue가 비어있을 경우 종료
        if (status->process_ready.empty()) {
            increase_cycle(status);
            return;
        }

        //다음 명령어 실행
        run_command(read_file(status->process_ready[0]->name).first[bookmarks[status->process_ready[0]->pid-1]], status);
        
    }
}



//main 함수
int main(int argc, char* argv[]) {

    //경로 지정
    string input_path(argv[1]);
//test용    string input_path = "/home/dongyeon/programs";
    directory_path = input_path;
    
    //result 파일 생성 및 초기화 후 열기
    file = fopen("result", "w");

    //init 프로세스 생성, status 초기화
    Process* process = create_process("init", 1, 0, 0);

    Status* status = new Status();
    status->cycle = -1;
    status->mode = "kernel";
    status->command = "boot";
    status->process_running = NULL;
    status->process_ready = {};
    status->process_waiting = {};
    status->process_new = process;
    status->process_terminated = NULL;

    //init 프로세스 실행
    run_command(read_file("init").first[bookmarks[0]], status);

    //result 파일 닫기
    delete status;
    fclose(file);

    return 0;
}
