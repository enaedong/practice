void run_command(string command, Status* status) {
    vector<string> command_list = split(command, ' ');
    string command_name = command_list[0];
    string arg1 = command_list[1];

    if (command_name == "run") {
        int cycle = stoi(arg1);
        status->process_running->cycle = cycle;
        status->process_running->mode = "user";
    } else if (command_name == "sleep") {
        int cycle = stoi(arg1);
        status->process_running->cycle = cycle;
        status->process_running->mode = "kernel";
        status->process_running->waiting_type = 's';
        add_process_to_waiting_queue(status->process_running, status);
        remove_process_from_running_queue(status->process_running, status);
    } else if (command_name == "fork_and_exec") {
        int pid = status->process_running->pid + 1;
        int ppid = status->process_running->pid;
        Process* process = create_process(arg1, pid, ppid);
        add_process_to_ready_queue(process, status);
        status->process_running->mode = "kernel";
    } else if (command_name == "wait") {
        status->process_running->mode = "kernel";
        status->process_running->waiting_type = 'w';
        add_process_to_waiting_queue(status->process_running, status);
        remove_process_from_running_queue(status->process_running, status);
    } else if (command_name == "exit") {
        status->process_running->mode = "kernel";
        add_process_to_terminated_queue(status->process_running, status);
        remove_process_from_running_queue(status->process_running, status);
    }
}
